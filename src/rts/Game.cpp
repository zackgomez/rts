#include "rts/Game.h"
#include <algorithm>
#include <sstream>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_real.hpp>
#include <boost/random/variate_generator.hpp>
#include "common/Checksum.h"
#include "common/ParamReader.h"
#include "common/util.h"
#include "rts/Actor.h"
#include "rts/GameEntity.h"
#include "rts/Map.h"
#include "rts/Player.h"
#include "rts/Renderer.h"
#include "rts/VisibilityMap.h"

namespace rts {

class GameRandom {
public:
  GameRandom(uint32_t seed)
    : generator_(seed),
      uni_(generator_, boost::uniform_real<>(0,1)),
      last_(0.f) {
  }

  float randomFloat() {
    return (last_ = uni_());
  }
  float getLastValue() {
    return last_;
  }

private:
  typedef boost::mt19937 base_generator_type;

  base_generator_type generator_;
  boost::variate_generator<base_generator_type&, boost::uniform_real<>> uni_;
  float last_;
};

Game* Game::instance_ = nullptr;

Game::Game(Map *map, const std::vector<Player *> &players)
  : map_(map),
    players_(players),
    tick_(0),
    tickOffset_(2),
    elapsedTime_(0.f),
    paused_(true),
    running_(true) {
  random_ = new GameRandom(45); // TODO(zack): pass in seed
  for (auto player : players) {
    player->setGame(this);
    teams_.insert(player->getTeamID());
    // Set up visibility map
    auto func = std::bind(&Player::visibleEntity, player, std::placeholders::_1);
    VisibilityMap *vismap = new VisibilityMap(
        map_->getSize(),
        func);
    visibilityMaps_[player->getPlayerID()] = vismap;
    requisition_[player->getPlayerID()] = 0.f;
  }
  // sort players to ensure consistency
  std::sort(
      players_.begin(), players_.end(),
      [](Player *p1, Player *p2) -> bool {
        return p1->getPlayerID() < p2->getPlayerID();
      });
  invariant(
      players_.size() <= map_->getMaxPlayers(),
      "too many players for map");

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
  delete random_;
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
    if (it.second->hasProperty(GameEntity::P_GAMEENTITY)) {
      static_cast<GameEntity *>(it.second)->checksum(chksum);
    }
  }

  TickChecksum ret;
  ret.entity_checksum = chksum.getChecksum();
  ret.action_checksum = actionChecksummer_.getChecksum();
  ret.random_checksum = random_->getLastValue();
  actionChecksummer_ = Checksum();

  return ret;
}

void Game::start() {
  // Lock the player update
  auto lock = std::unique_lock<std::mutex>(actionMutex_);
  pause();

  using namespace v8;
  script_.init();
  v8::Locker locker(script_.getIsolate());
  v8::Context::Scope context_scope(script_.getContext());
  
  HandleScope scope(script_.getIsolate());
  auto global = script_.getGlobal();

  float starting_requisition = fltParam("global.startingRequisition");
  for (int i = 0; i < players_.size(); i++) {
    auto *player = players_[i];
    auto starting_location = map_->getStartingLocation(i);
    Handle<Object> js_player_def = Object::New();
    js_player_def->Set(String::New("pid"), Integer::New(player->getPlayerID()));
    js_player_def->Set(String::New("tid"), Integer::New(player->getTeamID()));
    js_player_def->Set(
        String::New("starting_requisition"),
        Number::New(starting_requisition));
    js_player_def->Set(
        String::New("starting_location"),
        script_.jsonToJS(starting_location));

    TryCatch try_catch;
    const int argc = 1;
    Handle<Value> argv[argc] = { js_player_def };
    Handle<Object> playersAPI = Handle<Object>::Cast(
         global->Get(String::New("Players")));
    Handle<Function> playerInit = Handle<Function>::Cast(
          playersAPI->Get(String::New("playerInit")));
    auto ret = playerInit->Call(global, argc, argv);
    checkJSResult(ret, try_catch, "Players.playerInit");
  }

  // Initialize map
  map_->init();

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
  v8::Locker locker(script_.getIsolate());
  v8::Context::Scope context_scope(script_.getContext());
  v8::HandleScope scope(script_.getIsolate());

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

  elapsedTime_ += dt;

  // Don't allow new actions during this time
  std::unique_lock<std::mutex> actionLock(actionMutex_);

  // Do actions
  // It MUST be true that all players have added exactly one action of type
  // none targetting this frame
  tick_t idx = std::max((tick_t) 0, tick_ - tickOffset_);
  TickChecksum recordedChecksum = checksums_[idx];
  auto js_player_inputs = v8::Array::New();
  for (auto &player : players_) {
    id_t pid = player->getPlayerID();
    auto actions = player->getActions();
    std::vector<Json::Value> orders;

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
      } else if (action["type"] == ActionTypes::CHAT) {
        invariant(action.isMember("chat"), "malformed CHAT action");
        if (chatListener_) {
          chatListener_(pid, action);
        }
      } else if (action["type"] == ActionTypes::ORDER) {
        invariant(action.isMember("order"), "malformed ORDER action");
        actionChecksummer_.process(action);
        Json::Value order = action["order"];
        order["from_pid"] = toJson(pid);
        js_player_inputs->Set(
            js_player_inputs->Length(),
            script_.jsonToJS(order));
      } else {
        invariant_violation("unknown action type" + action["type"].asString());
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
      if (remoteChecksum.random_checksum != recordedChecksum.random_checksum) {
        LOG(ERROR) << tick_ << ": random checksum mismatch (theirs): "
          << remoteChecksum.random_checksum << " vs (ours): "
          << recordedChecksum.random_checksum << '\n';
      }
    }
  }
  // Allow more actions
  actionLock.unlock();

  // Update pathing
  map_->update(dt);

  std::vector<GameEntity *> entities;
  for (auto it : Renderer::get()->getEntities()) {
    if (it.second->hasProperty(GameEntity::P_GAMEENTITY)) {
      entities.push_back((GameEntity *)it.second);
    }
  }

  // player actions
  updateJS(js_player_inputs);


  // Integrate positions before updating entities, to ensure the render displays
  // extrapolated information.  This is safe and provides a better experience.
  for (auto entity : entities) {
    entity->integrate(dt);
  }

  // Update entities
  for (auto entity : entities) {
    entity->update(dt);
  }
  // Resolve entities
  for (auto entity : entities) {
    entity->resolve(dt);
  }
  // TODO: swap old/new entity states
  // clear messages
  clearJSMessages();

  // Remove deadEnts
  for (auto eid : deadEntities_) {
    script_.destroyEntity(eid);
    Renderer::get()->removeEntity(eid);
  }
  deadEntities_.clear();

  // Update players
  updateJSPlayers();

  entities.clear();
  for (auto it : Renderer::get()->getEntities()) {
    if (it.second->hasProperty(GameEntity::P_GAMEENTITY)) {
      entities.push_back((GameEntity *)it.second);
    }
  }

  // Collision detection
  for (auto it = entities.begin(); it != entities.end(); it++) {
    GameEntity *e1 = *it;
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
      GameEntity *e2 = *it2;
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
    for (auto entity : entities) {
      vit.second->processEntity(entity);
    }
  }

  // Synchronize with JS about resources/vps/etc
  renderJS();

  // TODO(zack): move this win condition into JS
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

float Game::gameRandom() {
  return random_->randomFloat();
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
  GameEntity *ent = new Actor(eid, name, params);
  if (ent) {
    Renderer::get()->spawnEntity(ent);
    script_.wrapEntity(ent, name, params);
  }
  return ent;
}

void Game::destroyEntity(id_t eid) {
  deadEntities_.insert(eid);
}

GameEntity * Game::getEntity(id_t eid) {
  auto entities = Renderer::get()->getEntities();
  auto it = entities.find(eid);
  if (it == entities.end() || !it->second->hasProperty(GameEntity::P_GAMEENTITY)) {
    return nullptr;
  }
  return (GameEntity *)it->second;
}

const GameEntity * Game::getEntity(id_t eid) const {
  auto entities = Renderer::get()->getEntities();
  auto it = entities.find(eid);
  if (it == entities.end() || !it->second->hasProperty(GameEntity::P_GAMEENTITY)) {
    return nullptr;
  }
  return (const GameEntity *)it->second;
}

const GameEntity * Game::findEntity(
    std::function<bool(const GameEntity *)> f) const {
  auto entities = Renderer::get()->getEntities();
  for (auto pair : entities) {
    if (!pair.second->hasProperty(GameEntity::P_GAMEENTITY)) {
      continue;
    }
    const GameEntity *e = (const GameEntity *)pair.second;
    if (f(e)) {
      return e;
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

float Game::getRequisition(id_t pid) const {
  auto it = requisition_.find(pid);
  invariant(it != requisition_.end(),
      "Unknown playerID to get requisition for");
  return it->second;
}

// Populate victoryPoints_ map from JS values.
void Game::renderJS() {
  using namespace v8;
  auto script = Game::get()->getScript();
  HandleScope scope(script->getIsolate());
  TryCatch try_catch;
  auto global = script->getGlobal();

  Handle<Object> game_object = Handle<Object>::Cast(
     global->Get(String::New("Game")));
  Handle<Function> game_render_function = Handle<Function>::Cast(
      game_object->Get(String::New("render")));
  Handle<Value> js_render_result_ret =
    game_render_function->Call(game_object, 0, nullptr);
  checkJSResult(js_render_result_ret, try_catch, "render");
  Handle<Object> js_render_result = Handle<Object>::Cast(js_render_result_ret);

  Handle<Array> js_players = Handle<Array>::Cast(
      js_render_result->Get(String::New("players")));
  for (int i = 0; i < js_players->Length(); i++) {
    Handle<Object> js_player = Handle<Object>::Cast(js_players->Get(i));
    id_t pid = js_player->Get(String::New("pid"))->IntegerValue();
    requisition_[pid] = js_player->Get(String::New("req"))->NumberValue();
  }

  Handle<Array> js_teams = Handle<Array>::Cast(
      js_render_result->Get(String::New("teams")));
  for (int i = 0; i < js_teams->Length(); i++) {
    auto js_team = Handle<Object>::Cast(js_teams->Get(i));
    id_t tid = js_team->Get(String::New("tid"))->IntegerValue();
    victoryPoints_[tid] = js_team->Get(String::New("vps"))->NumberValue();
  }
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

void Game::updateJS(v8::Handle<v8::Array> player_inputs) {
  using namespace v8;
  auto script = getScript();
  HandleScope scope(script->getIsolate());
  TryCatch try_catch;
  auto global = script->getGlobal();

  const int argc = 1;
  Handle<Value> argv[] = { player_inputs };

  Handle<Object> message_hub = Handle<Object>::Cast(
    global->Get(String::New("Game")));
  Handle<Value> ret =
    Handle<Function>::Cast(message_hub->Get(String::New("update")))
    ->Call(global, argc, argv);
  checkJSResult(ret, try_catch, "update:");
}

void Game::clearJSMessages() {
  using namespace v8;
  auto script = getScript();
  HandleScope scope(script->getIsolate());
  TryCatch try_catch;
  auto global = script->getGlobal();

  const int argc = 0;
  Handle<Value> *argv = nullptr;

  Handle<Object> message_hub = Handle<Object>::Cast(
    global->Get(String::New("MessageHub")));
  Handle<Value> ret =
    Handle<Function>::Cast(message_hub->Get(String::New("clearMessages")))
    ->Call(global, argc, argv);
  checkJSResult(ret, try_catch, "clearMessages:");
}

void Game::updateJSPlayers() {
  using namespace v8;
  auto script = getScript();
  HandleScope scope(script->getIsolate());
  TryCatch try_catch;
  auto global = script->getGlobal();

  const int argc = 0;
  Handle<Value> *argv = nullptr;

  Handle<Object> playersModule = Handle<Object>::Cast(
    global->Get(String::New("Players")));
  Handle<Value> ret =
    Handle<Function>::Cast(playersModule->Get(String::New("updateAllPlayers")))
    ->Call(global, argc, argv);
  checkJSResult(ret, try_catch, "updateAllPlayers:");
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
  ret["random"] = random_checksum;
  return ret;
}
};  // rts
