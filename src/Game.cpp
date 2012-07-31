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

namespace rts {

Game::Game(Map *map, const std::vector<Player *> &players) :
  map_(map),
  players_(players),
  tick_(0),
  tickOffset_(2),
  paused_(true),
  running_(true)
{
  logger_ = Logger::getLogger("Game");

  MessageHub::get()->setGame(this);

  for (auto player : players)
    player->setGame(this);
}

Game::~Game()
{
  MessageHub::get()->setGame(NULL);
  delete map_;
}

void Game::addRenderer(Renderer *renderer)
{
  renderers_.insert(renderer);
  renderer->setGame(this);
}

bool Game::updatePlayers()
{
  bool allInput = true;
  for (auto &player : players_)
  {
    allInput &= player->update(tick_);
  }
  return allInput;
}

void Game::start(float period)
{
  paused_ = true;
  // Synch up at start of game
  // Initially all players are targetting tick 0, we need to get them targetting
  // tickOffset_, so when we run frame 0, they're generating input for frame
  // tickOffset_
  for (tick_ = -tickOffset_+1; tick_ < 0; )
  {
    logger_->info() << "Running synch frame " << tick_ << '\n';

    // Lock the player update
    std::unique_lock<std::mutex> lock(mutex_);
    // See if everyone is ready to go
    if (updatePlayers())
      tick_++;

    SDL_Delay(int(1000*period));
  }

  // Initialize map
  map_->init();

  // Game is ready to go!
  paused_ = false;
}

void Game::update(float dt)
{
  // lock
  std::unique_lock<std::mutex> lock(mutex_);
  //logger_->info() << "Simulating tick " << tick_ << '\n';

  // First update players
  bool playersReady = updatePlayers();
  // If all the players aren't ready, we can't continue
  if (!playersReady)
  {
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
  for (auto &player : players_)
  {
    id_t pid = player->getPlayerID();
    assertPid(pid);
    std::queue<PlayerAction> &pacts = actions_[pid];

    PlayerAction act;
    for (;;)
    {
      assert(!pacts.empty());
      act = pacts.front(); pacts.pop();
      if (act["tick"] != (Json::Value::Int64) tick_)
      {
        logger_->fatal() << "Got bad action @ tick " << tick_
          << " action: " << act << '\n';
        assert (false);
      }

      if (act["type"] == ActionTypes::DONE)
        break;

      handleAction(pid, act);
    }
  }
  // Allow more actions
  actionLock.unlock();

  // Update entities
  std::vector<id_t> deadEnts;
  for (auto &it : entities_)
  {
    Entity *entity = it.second;
    entity->update(dt);
    // Check for removal
    if (entity->needsRemoval())
      deadEnts.push_back(entity->getID());
  }
  // Remove deadEnts
  for (auto eid : deadEnts)
  {
    Entity *e = entities_[eid];
    entities_.erase(eid);
    delete e;
  }
  // unlock entities automatically when lock goes out of scope

  // Next tick
  tick_++;
  sync_tick_ = SDL_GetTicks();
}

void Game::render(float dt)
{
  // lock
  std::unique_lock<std::mutex> lock(mutex_);

  dt = (SDL_GetTicks() - sync_tick_) / 1000.f;
  // Render
  for (auto &renderer : renderers_)
    renderer->startRender(dt);

  for (auto &renderer : renderers_)
    renderer->renderMap(map_);

  for (auto &it : entities_)
    for (auto &renderer : renderers_)
      renderer->renderEntity(it.second);

  // unlock
  lock.unlock();

  for (auto &renderer : renderers_)
    renderer->endRender();
}

void Game::sendMessage(id_t to, const Message &msg)
{
  assertEid(to);
  auto it = entities_.find(to);
  if (it == entities_.end())
  {
    logger_->warning() << "Tried to send message to unknown entity:\n" 
      << msg.toStyledString() << '\n';
    return;
  }

  it->second->handleMessage(msg);
}

void Game::handleMessage(const Message &msg)
{
  if (msg["type"] == MessageTypes::SPAWN_ENTITY)
  {
    assert(msg.isMember("entity_class"));
    assert(msg.isMember("entity_name"));

    Entity *ent = EntityFactory::get()->construct(
        msg["entity_class"].asString(),
        msg["entity_name"].asString(),
        msg);
    if (ent)
      entities_[ent->getID()] = ent;
  }
  else
  {
    logger_->warning() << "Game received unknown message type: " << msg;
  }
}

void Game::addAction(id_t pid, const PlayerAction &act)
{
  // CAREFUL: this function is called from different threads
  assert(act.isMember("type"));
  assertPid(pid);
  assert(getPlayer(pid));

  // Quit game, no more loops after this, broadcast the message to all other
  // players too
  if (act["type"] == ActionTypes::LEAVE_GAME)
  {
    running_ = false;
    logger_->info() << "Player " << pid << " has left the game.\n";
  }
  // Handle most events by just queueing them
  else
  {
    assert(act.isMember("tick"));
    std::unique_lock<std::mutex> lock(actionMutex_);
    actions_[pid].push(act);
    lock.unlock();
  }

  // Broadcast action to all players
  for (auto& player : players_)
    player->playerAction(pid, act);
}

const Entity * Game::getEntity(id_t eid) const
{
  auto it = entities_.find(eid);
  return it == entities_.end() ? NULL : it->second;
}

const Player * Game::getPlayer(id_t pid) const
{
  assertPid(pid);
  for (auto player : players_)
    if (player->getPlayerID() == pid)
      return player;

  return NULL;
}

void Game::handleAction(id_t playerID, const PlayerAction &action)
{
  // TODO(zack) include player ID in messages
  if (action["type"] == ActionTypes::MOVE)
  {
    // Generate a message to target entity with move order
    Message msg;
    msg["to"] = action["entity"];
    msg["type"] = MessageTypes::ORDER;
    msg["order_type"] = OrderTypes::MOVE;
    msg["target"] = action["target"];

    MessageHub::get()->sendMessage(msg);
  }
  else if (action["type"] == ActionTypes::ATTACK)
  {
    // Generate a message to target entity with attack order
    Message msg;
    msg["to"] = action["entity"];
    msg["type"] = MessageTypes::ORDER;
    msg["order_type"] = OrderTypes::ATTACK;
    if (action.isMember("target"))
      msg["target"] = action["target"];
    if (action.isMember("enemy_id"))
      msg["enemy_id"] = action["enemy_id"];

    MessageHub::get()->sendMessage(msg);
  }
  else if (action["type"] == ActionTypes::STOP)
  {
    Message msg;
    msg["to"] = action["entity"];
    msg["type"] = MessageTypes::ORDER;
    msg["order_type"] = OrderTypes::STOP;

    MessageHub::get()->sendMessage(msg);
  }
  else if (action["type"] == ActionTypes::ENQUEUE)
  {
    Message msg;
    msg["to"] = action["entity"];
    msg["type"] = MessageTypes::ORDER;
    msg["order_type"] = OrderTypes::ENQUEUE;
    msg["prod"] = action["prod"];

    MessageHub::get()->sendMessage(msg);
  }
  else
  {
    logger_->warning()
      << "Unknown action type from player " << playerID << " : " << action["type"].asString() << '\n';
  }
}

}; // rts
