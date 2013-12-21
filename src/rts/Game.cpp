#include "rts/Game.h"
#include <algorithm>
#include <sstream>
#include "common/ParamReader.h"
#include "common/util.h"
#include "rts/GameEntity.h"
#include "rts/Map.h"
#include "rts/Player.h"
#include "rts/Renderer.h"

namespace rts {

Game* Game::instance_ = nullptr;

Game::Game(Map *map, const std::vector<Player *> &players)
  : map_(map),
    players_(players),
    elapsedTime_(0.f),
    running_(true) {
  std::set<int> team_ids;
  for (auto player : players) {
    player->setGame(this);
    requisition_[player->getPlayerID()] = 0.f;
    team_ids.insert(player->getTeamID());
  }
  for (int tid : team_ids) {
    victoryPoints_[tid] = 0.f;
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

  assert(!instance_);
  instance_ = this;
}

Game::~Game() {
  delete map_;
  instance_ = nullptr;

}

v8::Handle<v8::Object> Game::getGameObject() {
  auto obj = script_.getInitReturn();
  invariant(
      obj->IsObject(),
      "expected js main function to return object");
  return v8::Handle<v8::Object>::Cast(obj);
}

void Game::start() {
  using namespace v8;
  script_.init("game-main");
  ENTER_GAMESCRIPT(&script_);

  auto game_object = getGameObject();

  Handle<Array> js_player_defs = Array::New();
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
        jsonToJS(starting_location));

    js_player_defs->Set(js_player_defs->Length(), js_player_def);
  }

  TryCatch try_catch;
  const int argc = 2;
  Handle<Value> argv[argc] = {
    jsonToJS(map_->getMapDefinition()),
    js_player_defs,
  };
  Handle<Function> game_init_method = Handle<Function>::Cast(
      game_object->Get(String::New("init")));
  auto ret = game_init_method->Call(game_object, argc, argv);
  checkJSResult(ret, try_catch, "Game.init:");
}

void Game::update(float dt) {
  ENTER_GAMESCRIPT(&script_);

  elapsedTime_ += dt;

  // Don't allow new actions during this time
  std::unique_lock<std::mutex> actionLock(actionMutex_);
  // Do actions
  auto js_player_inputs = v8::Array::New();
  for (const auto action : actions_) {
    js_player_inputs->Set(
        js_player_inputs->Length(),
        jsonToJS(action));

  }
  actions_.clear();
  // Allow more actions
  actionLock.unlock();

  // Update javascript, passing player input
  updateJS(js_player_inputs, dt);

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
}

// Reconcile javascript state with engine state
void Game::render() {
  using namespace v8;
  auto script = getScript();
  ENTER_GAMESCRIPT(script);
  TryCatch try_catch;

  auto game_object = getGameObject();
  Handle<Function> game_render_function = Handle<Function>::Cast(
      game_object->Get(String::New("render")));
  Handle<Value> js_render_result_ret =
    game_render_function->Call(game_object, 0, nullptr);
  checkJSResult(js_render_result_ret, try_catch, "render");
  Handle<String> js_render_result = Handle<String>::Cast(js_render_result_ret);
  char* encoded_render = *String::Utf8Value(js_render_result);
  char* encoded_end = encoded_render + js_render_result->Utf8Length();

  Json::Value json_render;
  Json::Reader().parse(encoded_render, encoded_end, json_render);
  renderFromJSON(json_render);
}

void UIActionFromJSON(const Json::Value &v) {
  UIAction uiaction;
  uiaction.render_id = e->getID();
  uiaction.owner_id = e->getGameID();
  uiaction.name = *String::AsciiValue(jsaction->Get(name));
  uiaction.icon = *String::AsciiValue(jsaction->Get(icon));
  std::string hotkey_str = *String::AsciiValue(jsaction->Get(hotkey));
  uiaction.hotkey = !hotkey_str.empty() ? hotkey_str[0] : '\0';
  uiaction.tooltip = *String::AsciiValue(jsaction->Get(tooltip));
  uiaction.targeting = static_cast<UIAction::TargetingType>(
                                                            jsaction->Get(targeting)->IntegerValue());
  uiaction.range = jsaction->Get(range)->NumberValue();
  uiaction.radius = jsaction->Get(radius)->NumberValue();
  uiaction.state = static_cast<UIAction::ActionState>(
                                                      jsaction->Get(state)->Uint32Value());
  uiaction.cooldown = jsaction->Get(cooldown)->NumberValue();
}

void renderEntityFromJSON(GameEntity *e, const Json::Value &v) {
  invariant(e, "must have entity to render to");
  if (v.isMember("alive")) {
    for (auto &sample : v["alive"]) {
      e->setAlive(sample[0].asFloat(), sample[1].asBool());
    }
  }
  if (v.isMember("model")) {
    for (auto &sample : v["model"]) {
      e->setModelName(sample[1].asString());
    }
  }
  if (v.isMember("properties")) {
    for (auto &sample : v["properties"]) {
      for (auto &prop : sample[1]) {
        e->addProperty(prop.asInt());
      }
    }
  }
  if (v.isMember("pid")) {
    for (auto &sample : v["pid"]) {
      e->setPlayerID(sample[0].asFloat(), toID(sample[1]));
    }
  }
  if (v.isMember("tid")) {
    for (auto &sample : v["tid"]) {
      e->setTeamID(sample[0].asFloat(), toID(sample[1]));
    }
  }
  if (v.isMember("pos")) {
    for (auto &sample : v["pos"]) {
      e->setPosition(sample[0].asFloat(), toVec2(sample[1]));
    }
  }
  if (v.isMember("size")) {
    for (auto &sample : v["size"]) {
      e->setSize(sample[0].asFloat(), toVec3(sample[1]));
    }
  }
  if (v.isMember("angle")) {
    for (auto &sample : v["angle"]) {
      e->setAngle(sample[0].asFloat(), sample[1].asFloat());
    }
  }
  if (v.isMember("sight")) {
    for (auto &sample : v["sight"]) {
      e->setSight(sample[0].asFloat(), sample[1].asFloat());
    }
  }
  if (v.isMember("visible")) {
    for (auto &sample : v["visible"]) {
      float t = sample[0].asFloat();
      VisibilitySet set;
      for (auto pid : sample[1]) {
        set.insert(toID(pid));
      }
      e->setVisibilitySet(t, set);
    }
  }
  // TODO
}

void Game::renderFromJSON(const Json::Value &v) {
  auto entities = must_have_idx(v, "entities");
  invariant(entities.isObject(), "should have entities array");
  auto entity_keys = entities.getMemberNames();
  for (auto &eid : entity_keys) {
    // find or create GameEntity corresponding to the
    auto it = game_to_render_id.find(eid);
    auto *entity = [&]() {
      if (it == game_to_render_id.end()) {
        id_t new_id = Renderer::get()->newEntityID();
        auto game_entity = new GameEntity(new_id);
        game_entity->setGameID(eid);
        Renderer::get()->spawnEntity(game_entity);
        game_to_render_id[eid] = new_id;
        return game_entity;
      }
      return GameEntity::cast(Renderer::get()->getEntity(it->second));
    }();
    renderEntityFromJSON(entity, entities[eid]);
  }


  auto players = must_have_idx(v, "players");
  invariant(players.isArray(), "players must be array");
  for (int i = 0; i < players.size(); i++) {
    auto player = players[i];
    id_t pid = toID(must_have_idx(player, "pid"));
    requisition_[pid] = must_have_idx(player, "req").asFloat();
  }

  auto teams = must_have_idx(v, "teams");
  for (int i = 0; i < teams.size(); i++) {
    auto team = teams[i];
    id_t tid = toID(must_have_idx(team, "tid"));
    victoryPoints_[tid] = must_have_idx(team, "vps").asFloat();
  }
}

void Game::addAction(id_t pid, const PlayerAction &act) {
  // CAREFUL: this function is called from different threads
  invariant(act.isMember("type"),
      "malformed player action" + act.toStyledString());
  invariant(getPlayer(pid), "action from unknown player");

  if (act["type"] == ActionTypes::ORDER) {
    std::unique_lock<std::mutex> lock(actionMutex_);
    auto order = must_have_idx(act, "order");
    order["from_pid"] = toJson(pid);
    actions_.push_back(order);
  } else if (act["type"] == ActionTypes::LEAVE_GAME) {
    running_ = false;
  } else if (act["type"] == ActionTypes::CHAT) {
    LOG(WARN) << "Chat is unimplemented\n";
  } else {
    invariant_violation(std::string("Unknown action type ") + act["type"].asString());
  }
}

const GameEntity * Game::getEntity(const std::string &game_id) const {
  // LOL this is a stupid hack
  float t = Renderer::get()->getGameTime();
  // TODO this is horribly inefficient :-/
  return findEntity([=](const GameEntity *e) -> bool {
    return e->getGameID() == game_id && e->getAlive(t);
  });
}

const GameEntity * Game::findEntity(
    std::function<bool(const GameEntity *)> f) const {
  auto entities = Renderer::get()->getEntities();
  for (auto pair : entities) {
    const auto *e = GameEntity::cast(pair.second);
    if (!e) continue;
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

float Game::getVictoryPoints(id_t tid) const {
  auto it = victoryPoints_.find(assertTid(tid));
  invariant(it != victoryPoints_.end(),
      "Unknown teamID to get victory points for");
  return it->second;
}

void Game::updateJS(v8::Handle<v8::Array> player_inputs, float dt) {
  using namespace v8;
  auto script = getScript();
  HandleScope scope(script->getIsolate());
  TryCatch try_catch;
  auto game_object = getGameObject();

  const int argc = 2;
  Handle<Value> argv[] = {
    player_inputs,
    Number::New(dt),
  };

  Handle<Value> ret =
    Handle<Function>::Cast(game_object->Get(String::New("update")))
    ->Call(game_object, argc, argv);
  checkJSResult(ret, try_catch, "update:");
}
};  // rts
