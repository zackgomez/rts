#include "rts/Actor.h"
#include <v8.h>
#include "common/ParamReader.h"
#include "common/util.h"
#include "rts/Game.h"
#include "rts/Player.h"
#include "rts/ResourceManager.h"
#include "rts/Renderer.h"

namespace rts {

Actor::Actor(id_t id, const std::string &name, const Json::Value &params)
  : GameEntity(id, name, params) {

  setMeshName(strParam("model"));
  setScale(glm::vec3(fltParam("modelSize")));
  resetTexture();
}

Actor::~Actor() {
}

const std::vector<UIAction> &Actor::getActions() const {
  return actions_;
}

float Actor::getSight() const {
  // TODO(zack): cache this each frame
  using namespace v8;
  auto *script = Game::get()->getScript();
  HandleScope scope(script->getIsolate());

  Handle<Object> jsactor = script->getEntity(getID());
  return jsactor->Get(String::New("sight_"))->NumberValue();
}

void Actor::resetTexture() {
  const Player *player = Game::get()->getPlayer(getPlayerID());
  auto color = player ? player->getColor() : ::vec3Param("global.defaultColor");
  GLuint texture = hasParam("texture")
    ? ResourceManager::get()->getTexture(strParam("texture"))
    : 0;
  setMaterial(createMaterial(color, 10.f, texture));
}

void Actor::collide(const GameEntity *collider, float dt) {
  if (!hasProperty(P_MOBILE)) {
    return;
  }
  if (!collider->hasProperty(P_ACTOR)
      || collider->getPlayerID() != getPlayerID()) {
    return;
  }

  // if either entity is moving forward, just ignore the message
  if (getSpeed() != 0.f || collider->getSpeed() != 0.f) {
    return;
  }

  // get difference between positions at time of intersection
  auto diff = getPosition2(dt) - collider->getPosition2(dt);
  float overlap_dist = glm::length(diff);

  // seperate away, or randomly if exactly the same pos
  auto dir = (overlap_dist > 1e-6f)
    ? diff / overlap_dist
    : randDir2();

  float bumpSpeed = ::fltParam("global.bumpSpeed");
  addBumpVel(glm::vec3(dir * bumpSpeed, 0.f));
}

void Actor::resolve(float dt) {
  using namespace v8;
  auto *script = Game::get()->getScript();
  HandleScope scope(script->getIsolate());
  Handle<Object> global = script->getGlobal();
  TryCatch try_catch;

  const int argc = 2;
  Handle<Value> argv[argc] = {script->getEntity(getID()), Number::New(dt)};
  Handle<Value> result =
    Handle<Function>::Cast(global->Get(String::New("entityResolve")))
    ->Call(global, argc, argv);
  checkJSResult(result, try_catch.Exception(), "entityResolve:");

  if (!getPathQueue().empty()) {
    glm::vec2 targetPos(getPathQueue().front());
    float dist = glm::length(targetPos - getPosition2());
    float speed = getMaxSpeed();
    // rotate
    turnTowards(targetPos);
    // move
    // Set speed careful not to overshoot
    if (dist < speed * dt) {
      speed = dist / dt;
    }
    setSpeed(speed);
  }

  updateUIInfo();
  updateActions();
}

void Actor::handleOrder(const Message &order) {
  if (order["type"] == "ACTION") {
    invariant(order.isMember("action"), "missing action name");
    std::string action_name = order["action"].asString();
    handleAction(action_name, order);
  } else {
    using namespace v8;
    auto *script = Game::get()->getScript();
    HandleScope scope(script->getIsolate());
    Handle<Object> global = script->getGlobal();
    TryCatch try_catch;

    // TODO(zack): HACK! either pass vec2 or expect vec3
    Json::Value cleanorder = order;
    if (order.isMember("target")) {
      cleanorder["target"] = toJson(toVec2(order["target"]));
    }

    Handle<Object> jsorder = Handle<Object>::Cast(script->jsonToJS(cleanorder));

    const int argc = 2;
    Handle<Value> argv[argc] = {script->getEntity(getID()), jsorder};
    Handle<Value> result =
      Handle<Function>::Cast(global->Get(String::New("entityHandleOrder")))
      ->Call(global, argc, argv);
    checkJSResult(result, try_catch.Exception(), "entityHandleOrder:");
  }
}

void Actor::handleAction(const std::string &action_name, const Json::Value &order) {
  using namespace v8;
  auto *script = Game::get()->getScript();
  HandleScope scope(script->getIsolate());

  Handle<Object> global = script->getGlobal();
  TryCatch try_catch;
  Handle<Value> target = Undefined();
  if (order.isMember("target")) {
    target = script->jsonToJS(order["target"]);
  };

  const int argc = 3;
  Handle<Value> argv[argc] = {
    script->getEntity(getID()),
    String::New(action_name.c_str()),
    target};
  Handle<Value> result =
    Handle<Function>::Cast(global->Get(String::New("entityHandleAction")))
    ->Call(global, argc, argv);
  checkJSResult(result, try_catch.Exception(), "entityHandleAction:");

  return;
}

void Actor::update(float dt) {
  GameEntity::update(dt);

  using namespace v8;
  auto script = Game::get()->getScript();
  HandleScope scope(script->getIsolate());
  TryCatch try_catch;

  auto global = script->getGlobal();
  const int argc = 2;

  Handle<Value> argv[] = {script->getEntity(getID()), Number::New(dt)};

  Handle<Value> result =
    Handle<Function>::Cast(global->Get(String::New("entityUpdate")))
    ->Call(global, argc, argv);
  checkJSResult(result, try_catch.Exception(), "entityUpdate:");
}

void Actor::updateUIInfo() {
  memset(&uiInfo_, 0, sizeof(uiInfo_));

  using namespace v8;
  auto script = Game::get()->getScript();
  HandleScope scope(script->getIsolate());
  TryCatch try_catch;
  auto global = script->getGlobal();
  const int argc = 1;
  Handle<Value> argv[argc] = {script->getEntity(getID())};

  Handle<Value> ret =
    Handle<Function>::Cast(global->Get(String::New("entityGetUIInfo")))
    ->Call(global, argc, argv);
  checkJSResult(ret, try_catch.Exception(), "entityGetUIInfo:");

  auto jsinfo = ret->ToObject();
  auto prod = String::New("production");
  if (jsinfo->Has(prod)) {
    uiInfo_.production = script->jsToVec2(
        Handle<Array>::Cast(jsinfo->Get(prod)));
  }
  auto health = String::New("health");
  if (jsinfo->Has(health)) {
    uiInfo_.health = script->jsToVec2(
        Handle<Array>::Cast(jsinfo->Get(health)));
  }
  auto capture = String::New("capture");
  if (jsinfo->Has(capture)) {
    uiInfo_.capture = script->jsToVec2(
        Handle<Array>::Cast(jsinfo->Get(capture)));
  }
}

void Actor::updateActions() {
  actions_.clear();

  using namespace v8;
  auto script = Game::get()->getScript();
  HandleScope scope(script->getIsolate());
  TryCatch try_catch;
  auto global = script->getGlobal();
  const int argc = 1;
  Handle<Value> argv[argc] = {script->getEntity(getID())};

  Handle<Value> ret =
    Handle<Function>::Cast(global->Get(String::New("entityGetActions")))
    ->Call(global, argc, argv);
  checkJSResult(ret, try_catch.Exception(), "entityGetActions:");

  auto name = String::New("name");
  auto icon = String::New("icon");
  auto tooltip = String::New("tooltip");
  auto targeting = String::New("targeting");
  auto range = String::New("range");
  auto state = String::New("state");
  auto cooldown = String::New("cooldown");
  Handle<Array> jsactions = Handle<Array>::Cast(ret);
  for (int i = 0; i < jsactions->Length(); i++) {
    Handle<Object> jsaction = Handle<Object>::Cast(jsactions->Get(i));
    UIAction uiaction;
    uiaction.owner = getID();
    uiaction.name = *String::AsciiValue(jsaction->Get(name));
    uiaction.icon = *String::AsciiValue(jsaction->Get(icon));
    uiaction.tooltip = *String::AsciiValue(jsaction->Get(tooltip));
    uiaction.targeting = static_cast<UIAction::TargetingType>(
        jsaction->Get(targeting)->IntegerValue());
    uiaction.range = jsaction->Get(range)->NumberValue();
    uiaction.state = static_cast<UIAction::ActionState>(
        jsaction->Get(state)->Uint32Value());
    uiaction.cooldown = jsaction->Get(cooldown)->NumberValue();
    actions_.push_back(uiaction);
  }
}
};  // rts
