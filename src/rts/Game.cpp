#include "Game.h"
#include "Renderer.h"
#include "Map.h"
#include "Player.h"
#include "Entity.h"
#include "Unit.h"
#include "Building.h"
#include "MessageHub.h"
#include "Projectile.h"
#include "EntityFactory.h"
#include "util.h"
#include "ParamReader.h"
#include <algorithm>

namespace rts {

bool comparePlayerID(Player *p1, Player *p2) {
  return p1->getPlayerID() < p2->getPlayerID();
}

Game* Game::instance_ = NULL;

Game::Game(Map *map, const std::vector<Player *> &players,
    Renderer *renderer) :
  map_(map),
  players_(players),
  renderer_(renderer),
  tick_(0),
  tickOffset_(2),
  paused_(true),
  running_(true) {
  logger_ = Logger::getLogger("Game");

  MessageHub::get()->setGame(this);

  for (auto player : players) {
    player->setGame(this);
    // Add resources
    PlayerResources res;
    res.requisition = 0.f;
    resources_[player->getPlayerID()] = res;
  }
  // sort players to ensure consistency
  std::sort(players_.begin(), players_.end(), comparePlayerID);

  renderer_->setGame(this);

  assert(!instance_);
  instance_ = this;
}

Game::~Game() {
  MessageHub::get()->setGame(NULL);
  delete map_;
  instance_ = NULL;
}

bool Game::updatePlayers() {
  bool allInput = true;
  for (auto &player : players_) {
    allInput &= player->update(tick_);
  }
  return allInput;
}

void Game::start(float period) {
  paused_ = true;
  // Synch up at start of game
  // Initially all players are targetting tick 0, we need to get them targetting
  // tickOffset_, so when we run frame 0, they're generating input for frame
  // tickOffset_
  for (tick_ = -tickOffset_+1; tick_ < 0; ) {
    logger_->info() << "Running synch frame " << tick_ << '\n';

    // Lock the player update
    std::unique_lock<std::mutex> lock(mutex_);
    // See if everyone is ready to go
    if (updatePlayers()) {
      tick_++;
    }

    SDL_Delay(int(1000*period));
  }

  // Starting resources
  float startingReq = fltParam("global.startingRequisition");
  for (auto &player : players_) {
    resources_[player->getPlayerID()].requisition += startingReq;
  }

  // Initialize map
  map_->init(players_);

  // Game is ready to go!
  paused_ = false;
}

void Game::update(float dt) {
  // lock
  std::unique_lock<std::mutex> lock(mutex_);

  // First update players
  bool playersReady = updatePlayers();
  // If all the players aren't ready, we can't continue
  if (!playersReady) {
    logger_->warning() << "Not all players ready for tick " << tick_ << '\n';
    paused_ = true;
    return;
  }
  paused_ = false;

  // Don't allow new actions during this time
  std::unique_lock<std::mutex> actionLock(actionMutex_);

  // Do actions
  // It MUST be true that all players have added exactly one action of type
  // none targetting this frame
for (auto &player : players_) {
    id_t pid = player->getPlayerID();
    assertPid(pid);
    std::queue<PlayerAction> &pacts = actions_[pid];

    PlayerAction act;
    for (;;) {
      invariant(!pacts.empty(), "unexpected empty player action");
      act = pacts.front();
      pacts.pop();
      invariant(
        act["tick"] == toJson(tick_),
        "Bad action tick " + act.toStyledString()
      );

      if (act["type"] == ActionTypes::DONE) {
        break;
      }

      handleAction(pid, act);
    }

    // Check to see if this player has won
    if (resources_[pid].victory_points >= intParam("global.pointsToWin")) {
      logger_->info() << "Player " << pid << " has won the game!\n";
      // TODO(connor) do something cooler than exit here, like give them candy.
      // TODO(connor) also, send a message to game.
      running_ = false;
    }
  }
  // Allow more actions
  actionLock.unlock();

  // Update entities
  std::vector<id_t> deadEnts;
for (auto &it : entities_) {
    Entity *entity = it.second;
    entity->update(dt);
  }
  // Remove deadEnts
for (auto eid : deadEntities_) {
    Entity *e = entities_[eid];
    entities_.erase(eid);
    delete e;
  }
  deadEntities_.clear();
  // unlock entities automatically when lock goes out of scope

  // Next tick
  tick_++;
  sync_tick_ = SDL_GetTicks();
}

void Game::render(float dt) {
  // lock
  std::unique_lock<std::mutex> lock(mutex_);

  dt = (SDL_GetTicks() - sync_tick_) / 1000.f;

  renderer_->startRender(dt);

  renderer_->renderMessages(messages_);
  messages_.clear();

  renderer_->renderMap(map_);

  for (auto &it : entities_) {
    renderer_->renderEntity(it.second);
  }

  // unlock
  lock.unlock();

  renderer_->endRender();
}

void Game::sendMessage(id_t to, const Message &msg) {
  assertEid(to);
  auto it = entities_.find(to);
  if (it == entities_.end()) {
    logger_->warning() << "Tried to send message to unknown entity:\n"
                       << msg.toStyledString() << '\n';
    return;
  }

  it->second->handleMessage(msg);
  messages_.insert(msg);
}

void Game::handleMessage(const Message &msg) {
  invariant(msg.isMember("type"), "malformed message");
  if (msg["type"] == MessageTypes::SPAWN_ENTITY) {
    invariant(msg.isMember("entity_class"),
              "malformed SPAWN_ENTITY message");
    invariant(msg.isMember("entity_name"),
              "malformed SPAWN_ENTITY message");
    invariant(msg.isMember("params"),
              "malformed SPAWN_ENTITY message");

    Entity *ent = EntityFactory::get()->construct(
                    msg["entity_class"].asString(),
                    msg["entity_name"].asString(),
                    msg["params"]);
    if (ent) {
      entities_[ent->getID()] = ent;
    }
  } else if (msg["type"] == MessageTypes::DESTROY_ENTITY) {
    invariant(msg.isMember("eid"), "malformed DESTROY_ENTITY message");
    deadEntities_.push_back(toID(msg["eid"]));
  } else if (msg["type"] == MessageTypes::ADD_RESOURCE) {
    invariant(msg.isMember("pid"), "malformed ADD_RESOURCE message");
    invariant(msg.isMember("amount"), "malformed ADD_RESOURCE message");
    invariant(msg.isMember("resource"), "malformed ADD_RESOURCE message");
    id_t pid = toID(msg["pid"]);
    invariant(getPlayer(pid), "unknown player for ADD_RESOURCE message");
    float amount = msg["amount"].asFloat();
    if (msg["resource"] == ResourceTypes::REQUISITION) {
      resources_[pid].requisition += amount;
    } else if (msg["resource"] == ResourceTypes::VICTORY) {
      resources_[pid].victory_points += amount;
    }
  } else {
    logger_->warning() << "Game received unknown message type: " << msg;
    // No other work, if unknown message
    return;
  }

  messages_.insert(msg);
}

void Game::addAction(id_t pid, const PlayerAction &act) {
  // CAREFUL: this function is called from different threads
  invariant(act.isMember("type"), "malformed player action");
  assertPid(pid);
  invariant(getPlayer(pid), "action from unknown player");

  // Quit game, no more loops after this, broadcast the message to all other
  // players too
  if (act["type"] == ActionTypes::LEAVE_GAME) {
    running_ = false;
    logger_->info() << "Player " << pid << " has left the game.\n";
  }
  // Handle most events by just queueing them
  else {
    invariant(act.isMember("tick"), "missing tick in player action");
    std::unique_lock<std::mutex> lock(actionMutex_);
    actions_[pid].push(act);
    lock.unlock();
  }

  // Broadcast action to all players
  for (auto& player : players_) {
    player->playerAction(pid, act);
  }
}

const Entity * Game::getEntity(id_t eid) const {
  auto it = entities_.find(eid);
  return it == entities_.end() ? NULL : it->second;
}

const Player * Game::getPlayer(id_t pid) const {
  if (pid == NO_PLAYER) {
    return NULL;
  }

  assertPid(pid);
  for (auto player : players_) {
    if (player->getPlayerID() == pid) {
      return player;
    }
  }

  invariant(false, "Asked to find unknown pid!");
  return NULL;
}

const PlayerResources& Game::getResources(id_t pid) const {
  auto it = resources_.find(pid);
  invariant(it != resources_.end(),
      "Unknown playerID to get resources for");
  return it->second;
}

void Game::handleAction(id_t playerID, const PlayerAction &action) {
  if (action["type"] == ActionTypes::CHAT) {
    invariant(action.isMember("chat"), "malformed CHAT action");
    std::stringstream ss;
    ss << getPlayer(playerID)->getName() << ": ";
    ss << action["chat"].asString();

    // Tell all players that we are chatting..
    // TODO(connor) iterate over only a target set of players so we can do team
    // chat, whispers, etc. Define this target set as a member of msg.
    for (auto &player : players_) {
      player->addChatMessage(ss.str());
      player->displayChatWindow();
    }
  } else {
    Message msg;
    msg["to"] = action["entity"];
    msg["from"] = toJson(playerID);
    msg["type"] = MessageTypes::ORDER;
    if (action["type"] == ActionTypes::MOVE) {
      // Generate a message to target entity with move order
      msg["order_type"] = OrderTypes::MOVE;
      msg["target"] = action["target"];

      MessageHub::get()->sendMessage(msg);
    } else if (action["type"] == ActionTypes::ATTACK) {
      // Generate a message to target entity with attack order
      msg["order_type"] = OrderTypes::ATTACK;
      if (action.isMember("target")) {
        msg["target"] = action["target"];
      }
      if (action.isMember("enemy_id")) {
        msg["enemy_id"] = action["enemy_id"];
      }

      MessageHub::get()->sendMessage(msg);
    } else if (action["type"] == ActionTypes::CAPTURE) {
      msg["enemy_id"] = action["enemy_id"];
      msg["order_type"] = OrderTypes::CAPTURE;

      MessageHub::get()->sendMessage(msg);
    } else if (action["type"] == ActionTypes::STOP) {
      msg["order_type"] = OrderTypes::STOP;

      MessageHub::get()->sendMessage(msg);
    } else if (action["type"] == ActionTypes::ENQUEUE) {
      msg["order_type"] = OrderTypes::ENQUEUE;
      msg["prod"] = action["prod"];

      float cost = fltParam(msg["prod"].asString() + ".cost.requisition");
      invariant(cost <= resources_[playerID].requisition,
          "insufficient requsition to enqueue");
      resources_[playerID].requisition -= cost;

      MessageHub::get()->sendMessage(msg);
    } else {
      logger_->warning()
        << "Unknown action type from player " << playerID << " : " 
        << action["type"].asString() << '\n';
    }
  }
}
}; // rts
