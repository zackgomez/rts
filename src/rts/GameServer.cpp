#include "rts/GameServer.h"
#include "common/Logger.h"
#include "common/util.h"

namespace rts {

GameServer::GameServer()
  : running_(false) {
  script_ = new GameScript();
}

GameServer::~GameServer() {
  delete script_;
}

void GameServer::addAction(id_t pid, const PlayerAction &act) {
  // CAREFUL: this function is called from different threads
  invariant(act.isMember("type"),
      "malformed player action" + act.toStyledString());

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

void GameServer::updateJS(v8::Handle<v8::Array> player_inputs, float dt) {
  using namespace v8;
  auto script = script_;
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

v8::Handle<v8::Object> GameServer::getGameObject() {
  auto obj = script_->getInitReturn();
  invariant(
      obj->IsObject(),
      "expected js main function to return object");
  return v8::Handle<v8::Object>::Cast(obj);
}

void GameServer::start(const Json::Value &map_def, const Json::Value &player_defs) {
  using namespace v8;
  script_->init("game-main");
  ENTER_GAMESCRIPT(script_);

  auto game_object = getGameObject();

  TryCatch try_catch;
  const int argc = 2;
  Handle<Value> argv[argc] = {
    jsonToJS(map_def),
    jsonToJS(player_defs),
  };
  Handle<Function> game_init_method = Handle<Function>::Cast(
      game_object->Get(String::New("init")));
  auto ret = game_init_method->Call(game_object, argc, argv);
  checkJSResult(ret, try_catch, "Game.init:");

  running_ = true;
}

Json::Value GameServer::update(float dt) {
  using namespace v8;
  ENTER_GAMESCRIPT(script_);
  auto game_object = getGameObject();

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

  TryCatch try_catch;
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
  return json_render;
}
};