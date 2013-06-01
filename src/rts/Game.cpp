#include "rts/Game.h"
#include <algorithm>
#include <sstream>
#include <SDL/SDL.h>
#include "common/Checksum.h"
#include "common/ParamReader.h"
#include "common/util.h"
#include "rts/GameEntity.h"
#include "rts/EntityFactory.h"
#include "rts/Map.h"
#include "rts/MessageHub.h"
#include "rts/Player.h"
#include "rts/Renderer.h"
#include "rts/Unit.h"
#include "rts/VisibilityMap.h"

namespace rts {

Game* Game::instance_ = nullptr;

Game::Game(Map *map, const std::vector<Player *> &players)
  : map_(map),
    players_(players),
    tick_(0),
    tickOffset_(2),
    paused_(true),
    running_(true) {
  MessageHub::get()->setGame(this);

  for (auto player : players) {
    player->setGame(this);
    teams_.insert(player->getTeamID());
    // Add resources
    PlayerResources res;
    res.requisition = 0.f;
    resources_[player->getPlayerID()] = res;
    // Set up visibility map
    auto func = std::bind(&Player::visibleEntity, player, std::placeholders::_1);
    VisibilityMap *vismap = new VisibilityMap(
        map_->getSize(),
        func);
    visibilityMaps_[player->getPlayerID()] = vismap;
  }
  // sort players to ensure consistency
  std::sort(
      players_.begin(), players_.end(),
      [](Player *p1, Player *p2) -> bool {
        return p1->getPlayerID() < p2->getPlayerID();
      });

  // Team init
  invariant(teams_.size() == 2, "game only supports exactly 2 teams");
  for (id_t tid : teams_) {
    assertTid(tid);
    victoryPoints_[tid] = 0.f;
  }

  assert(!instance_);
  instance_ = this;
}

Game::~Game() {
  MessageHub::get()->setGame(nullptr);
  delete map_;
  for (auto map : visibilityMaps_) {
    delete map.second;
  }
  instance_ = nullptr;

}

bool Game::updatePlayers() {
  for (auto player : players_) {
    if (!player->isReady()) {
      return false;
    }
  }
  return true;
}

TickChecksum Game::checksum() {
  Checksum chksum;
  for (auto it : Renderer::get()->getEntities()) {
    it.second->checksum(chksum);
  }

  TickChecksum ret;
  ret.entity_checksum = chksum.getChecksum();
  ret.action_checksum = actionChecksummer_.getChecksum();
  actionChecksummer_ = Checksum();

  return ret;
}

void Game::start() {
  // Lock the player update
  auto lock = std::unique_lock<std::mutex>(actionMutex_);
  pause();

  script_.init();
  v8::Locker locker(script_.getIsolate());
  v8::Context::Scope context_scope(script_.getContext());

  // Starting resources
  // TODO(zack): move to map init (spawn logical entities with correct
  // values)
  float startingReq = fltParam("global.startingRequisition");
  for (auto &player : players_) {
    resources_[player->getPlayerID()].requisition += startingReq;
  }

  // Initialize map
  map_->init(players_);

  checksums_.push_back(checksum());

  // Queue up offset number of 'done' frames
  for (tick_ = -tickOffset_; tick_ < 0; tick_++) {
    LOG(INFO) << "Running sync tick " << tick_ << '\n';
    for (auto player : players_) {
      player->startTick(tick_);
    }
  }

  // Game is ready to go!
  unpause();
}

void Game::update(float dt) {
  auto lock = Renderer::get()->lockEngine();
  auto& entities = Renderer::get()->getEntities();
  v8::Locker locker(script_.getIsolate());
  v8::Context::Scope context_scope(script_.getContext());

  // First update players
  // TODO(zack): less hacky dependence on paused_
  if (!paused_) {
    for (auto player : players_) {
      player->startTick(tick_);
    }
  }

  // If all the players aren't ready, we can't continue
  bool playersReady = updatePlayers();
  if (!playersReady) {
    LOG(WARN) << "Not all players ready for tick " << tick_ << '\n';
    pause();
    return;
  }
  paused_ = false;
  unpause();

  // Don't allow new actions during this time
  std::unique_lock<std::mutex> actionLock(actionMutex_);

  // Do actions
  // It MUST be true that all players have added exactly one action of type
  // none targetting this frame
  tick_t idx = std::max((tick_t) 0, tick_ - tickOffset_);
  TickChecksum recordedChecksum = checksums_[idx];
  for (auto &player : players_) {
    id_t pid = player->getPlayerID();
    auto actions = player->getActions();

    TickChecksum remoteChecksum;
    bool done = false;
    for (const auto &action : actions) {
      invariant(!done, "Action after DONE message??");
      if (action["type"] == ActionTypes::DONE) {
        // Check for sync error
        // Index into checksum array, [0, n]
        remoteChecksum = TickChecksum(action["checksum"]);
        done = true;
        invariant(
            toTick(action["tick"]) == (tick_ - tickOffset_),
            "tick mismatch");
      } else if (action["type"] == ActionTypes::LEAVE_GAME) {
        running_ = false;
        return;
      } else {
        actionChecksummer_.process(action);
        handleAction(pid, action);
      }
    }
    invariant(done, "Missing DONE message for player");
    // TODO(zack): make this dump out a log so we can debug the reason why
    if (TickChecksum(remoteChecksum) != recordedChecksum) {
      if (remoteChecksum.entity_checksum != recordedChecksum.entity_checksum) {
        LOG(ERROR) << tick_ << ": entity checksum mismatch (theirs): "
          << remoteChecksum.entity_checksum << " vs (ours): "
          << recordedChecksum.entity_checksum << '\n';
      }
      if (remoteChecksum.action_checksum != recordedChecksum.action_checksum) {
        LOG(ERROR) << tick_ << ": action checksum mismatch (theirs): "
          << remoteChecksum.action_checksum << " vs (ours): "
          << recordedChecksum.action_checksum << '\n';
      }
    }
  }
  // Allow more actions
  actionLock.unlock();

  // Update pathing
  map_->update(dt);

  // Integrate positions before updating entities, to ensure the render displays
  // extrapolated information.  This is safe and provides a better experience.
  for (auto &it : entities) {
    it.second->integrate(dt);
  }

  // Update entities
  for (auto &it : entities) {
    GameEntity *entity = it.second;
    entity->update(dt);
  }
  // Remove deadEnts
  for (auto eid : deadEntities_) {
    script_.destroyEntity(getEntity(eid));
    Renderer::get()->removeEntity(eid);
  }
  deadEntities_.clear();

  // Collision detection
  for (auto it = entities.begin(); it != entities.end(); it++) {
    GameEntity *e1 = it->second;
    if (!e1->hasProperty(GameEntity::P_COLLIDABLE)) {
      continue;
    }
    // Collide with map
    e1->setPosition(
      glm::vec3(
        glm::vec2(e1->getPosition()),
        map_->getMapHeight(glm::vec2(e1->getPosition()))));
    auto it2 = it;
    for (it2++; it2 != entities.end(); it2++) {
      GameEntity *e2 = it2->second;
      if (!e2->hasProperty(GameEntity::P_COLLIDABLE)) {
        continue;
      }
      float time;
      if ((time = boxBoxCollision(
            e1->getRect(),
            glm::vec2(e1->getVelocity()),
            e2->getRect(),
            glm::vec2(e2->getVelocity()),
            dt)) != NO_INTERSECTION) {
        e1->collide(e2, time);
        e2->collide(e1, time);
      }
    }
  }

  for (auto &vit : visibilityMaps_) {
    vit.second->clear();
    for (auto &it : entities) {
      GameEntity *entity = it.second;
      vit.second->processEntity(entity);
    }
  }

  // Check to see if this player has won
  for (auto it : victoryPoints_) {
    if (it.second > intParam("global.pointsToWin")) {
      // TODO(zack): print out each player on that team at least...
      LOG(INFO) << "Team " << it.first << " has won the game!\n";
      // TODO(connor) do something cooler than exit here, like give them candy.
      // TODO(connor) also, send a message to game
      running_ = false;
    }
  }

  // Checksum the tick
  checksums_.push_back(checksum());

  // unlock entities automatically when lock goes out of scope
  // Next tick
  tick_++;
  Renderer::get()->setLastTickTime(Clock::now());

  // unlock game automatically when lock goes out of scope
}

void Game::sendMessage(id_t to, const Message &msg) {
  auto entities = Renderer::get()->getEntities();
  assertEid(to);
  auto it = entities.find(to);
  if (it == entities.end()) {
    LOG(WARN) << "Tried to send message to unknown entity:\n"
      << msg.toStyledString() << '\n';
    return;
  }

  it->second->handleMessage(msg);
}

void Game::addResources(
    id_t pid, ResourceType resource_type, float amount, id_t from) {
  invariant(getPlayer(pid), "unknown player for ADD_RESOURCE message");
  if (resource_type == ResourceType::REQUISITION) {
    resources_[pid].requisition += amount;
  }
}

void Game::addVPs(id_t tid, float amount, id_t from) {
  assertTid(tid);
  invariant(victoryPoints_.count(tid), "unknown team for vp add");
    victoryPoints_[tid] += amount;
}

void Game::addAction(id_t pid, const PlayerAction &act) {
  // CAREFUL: this function is called from different threads
  invariant(act.isMember("type"),
      "malformed player action" + act.toStyledString());
  invariant(getPlayer(pid), "action from unknown player");

  // Broadcast action to all players
  for (auto& player : players_) {
    player->playerAction(pid, act);
  }
}

const GameEntity * Game::spawnEntity(
    const std::string &name,
    const Json::Value &params) {
  id_t eid = Renderer::get()->newEntityID();
  GameEntity *ent = EntityFactory::get()->construct(eid, name, params);
  if (ent) {
    Renderer::get()->spawnEntity(ent);
    script_.wrapEntity(ent, params);
  }
  return ent;
}

void Game::destroyEntity(id_t eid) {
  deadEntities_.insert(eid);
}

GameEntity * Game::getEntity(id_t eid) {
  auto entities = Renderer::get()->getEntities();
  auto it = entities.find(eid);
  return it == entities.end() ? nullptr : it->second;
}

const GameEntity * Game::getEntity(id_t eid) const {
  auto entities = Renderer::get()->getEntities();
  auto it = entities.find(eid);
  return it == entities.end() ? nullptr : it->second;
}

const GameEntity * Game::findEntity(
    std::function<bool(const GameEntity *)> f) const {
  auto entities = Renderer::get()->getEntities();
  for (auto pair : entities) {
    if (f(pair.second)) {
      return pair.second;
    }
  }

  return nullptr;
}

const Player * Game::getPlayer(id_t pid) const {
  if (pid == NO_PLAYER) {
    return nullptr;
  }

  assertPid(pid);
  for (auto player : players_) {
    if (player->getPlayerID() == pid) {
      return player;
    }
  }

  invariant(false, "Asked to find unknown pid!");
  return nullptr;
}

const PlayerResources& Game::getResources(id_t pid) const {
  auto it = resources_.find(pid);
  invariant(it != resources_.end(),
      "Unknown playerID to get resources for");
  return it->second;
}

float Game::getVictoryPoints(id_t tid) const {
  auto it = victoryPoints_.find(assertTid(tid));
  invariant(it != victoryPoints_.end(),
      "Unknown teamID to get victory points for");
  return it->second;
}

const VisibilityMap* Game::getVisibilityMap(id_t pid) const {
  auto it = visibilityMaps_.find(pid);
  invariant(it != visibilityMaps_.end(),
      "Unknown playerID to get resources for");
  return it->second;
}

void Game::handleAction(id_t playerID, const PlayerAction &action) {
  if (action["type"] == ActionTypes::CHAT) {
    invariant(action.isMember("chat"), "malformed CHAT action");
    if (chatListener_) {
      chatListener_(action);
    }
  } else {
    Message msg;
    msg["to"] = action["entity"];
    msg["from"] = toJson(playerID);
    msg["type"] = MessageTypes::ORDER;
    if (action["type"] == ActionTypes::MOVE) {
      // Generate a message to target entity with move ordervi
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
    } else if (action["type"] == ActionTypes::ACTION) {
      msg["order_type"] = OrderTypes::ACTION;
      msg["action"] = action["action"];
      msg["target"] = action["target"];

      MessageHub::get()->sendMessage(msg);
    } else {
      LOG(WARN)
        << "Unknown action type from player " << playerID << " : "
        << action["type"].asString() << '\n';
    }
  }
}

void Game::pause() {
  paused_ = true;
  Renderer::get()->setTimeMultiplier(0.f);
}

void Game::unpause() {
  paused_ = false;
  Renderer::get()->setTimeMultiplier(1.f);
}

TickChecksum::TickChecksum(const Json::Value &val) {
  invariant(
    val.isMember("entity") && val.isMember("action"),
    "malformed checksum");
  entity_checksum = val["entity"].asUInt();
  action_checksum = val["action"].asUInt();
}
bool TickChecksum::operator==(const TickChecksum &rhs) const {
  return entity_checksum == rhs.entity_checksum
    && action_checksum == rhs.action_checksum;
}
bool TickChecksum::operator!=(const TickChecksum &rhs) const {
  return !(*this == rhs);
}
Json::Value TickChecksum::toJson() const {
  Json::Value ret;
  ret["action"] = action_checksum;
  ret["entity"] = entity_checksum;
  return ret;
}
};  // rts
