#include "rts/Actor.h"
#include "common/ParamReader.h"
#include "common/util.h"
#include "rts/MessageHub.h"
#include "rts/Player.h"
#include "rts/Projectile.h"
#include "rts/ResourceManager.h"
#include "rts/Unit.h"
#include "rts/Weapon.h"
#include "rts/Renderer.h"

namespace rts {

Actor::Actor(id_t id, const std::string &name, const Json::Value &params)
  : GameEntity(id, name, params),
    melee_timer_(0.f),
    meleeWeapon_(nullptr),
    rangedWeapon_(nullptr) {
  setProperty(P_ACTOR, true);

  if (hasParam("meleeWeapon")) {
    meleeWeapon_ = new Weapon(strParam("meleeWeapon"), this);
  }
  if (hasParam("rangedWeapon")) {
    rangedWeapon_ = new Weapon(strParam("rangedWeapon"), this);
  }

  setMeshName(strParam("model"));
  setScale(glm::vec3(fltParam("modelSize")));
  resetTexture();

  if (hasParam("actions")) {
    auto actions = getParam("actions");
    for (int i = 0; i < actions.size(); i++) {
      actions_.emplace_back(getID(), actions[i]);
    }
  }
}

Actor::~Actor() {
}

const std::vector<UIAction> &Actor::getActions() const {
  return actions_;
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

void Actor::handleMessage(const Message &msg) {
  if (msg["type"] == MessageTypes::ORDER) {
    handleOrder(msg);
  } else {
    using namespace v8;
    auto *script = Game::get()->getScript();
    HandleScope scope(script->getIsolate());
    Handle<Object> global = script->getGlobal();
    TryCatch try_catch;

    Handle<Object> jsmsg = Handle<Object>::Cast(script->jsonToJS(msg));

    const int argc = 2;
    Handle<Value> argv[argc] = {script->getEntity(getID()), jsmsg};
    Handle<Value> result =
      Handle<Function>::Cast(global->Get(String::New("entityHandleMessage")))
      ->Call(global, argc, argv);
    if (result.IsEmpty()) {
      LOG(ERROR) << "error handling action: "
        << *String::AsciiValue(try_catch.Exception()) << '\n';
    }
  }
}

void Actor::handleOrder(const Message &order) {
  invariant(order["type"] == MessageTypes::ORDER, "unknown message type");
  invariant(order.isMember("order_type"), "missing order type");
  if (order["order_type"] == OrderTypes::ACTION) {
    invariant(order.isMember("action"), "missing action name");
    std::string action_name = order["action"].asString();
    for (int i = 0; i < actions_.size(); i++) {
      if (actions_[i].getName() == action_name) {
        handleAction(actions_[i].getRawDefinition());
        return;
      }
    }
    LOG(WARNING) << "Actor got unknown action order: "
      << order.toStyledString() << '\n';
  } else {
    LOG(WARNING) << "Actor got unknown order: "
      << order.toStyledString() << '\n';
  }
}

void Actor::handleAction(const Json::Value &action) {
  using namespace v8;
  auto *script = Game::get()->getScript();
  HandleScope scope(script->getIsolate());

  Handle<Object> jsaction = Handle<Object>::Cast(script->jsonToJS(action));

  Handle<Object> global = script->getGlobal();
  TryCatch try_catch;

  const int argc = 2;
  Handle<Value> argv[argc] = {script->getEntity(getID()), jsaction};
  Handle<Value> result =
    Handle<Function>::Cast(global->Get(String::New("entityHandleAction")))
    ->Call(global, argc, argv);
  if (result.IsEmpty()) {
    LOG(ERROR) << "error handling action: "
      << *String::AsciiValue(try_catch.Exception()) << '\n';
  }

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

  Handle<Value> ret =
    Handle<Function>::Cast(global->Get(String::New("entityUpdate")))
    ->Call(global, argc, argv);
  if (ret.IsEmpty()) {
    LOG(ERROR) << "Error updating entity: "
      << *String::AsciiValue(try_catch.Exception()) << '\n';
  }

  melee_timer_ -= dt;

  updateUIInfo();
}

float Actor::distanceToEntity(const GameEntity *e) const {
  return rayBox2Intersection(
    getPosition2(),
    glm::normalize(e->getPosition2() - getPosition2()),
    e->getRect());
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
  if (ret.IsEmpty()) {
    LOG(ERROR) << "Error updating ui info entity: "
      << *String::AsciiValue(try_catch.Exception()) << '\n';
    return;
  }

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
};  // rts
