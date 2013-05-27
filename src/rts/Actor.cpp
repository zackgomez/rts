#include "rts/Actor.h"
#include "common/ParamReader.h"
#include "common/util.h"
#include "rts/Building.h"
#include "rts/MessageHub.h"
#include "rts/Player.h"
#include "rts/Projectile.h"
#include "rts/ResourceManager.h"
#include "rts/Unit.h"
#include "rts/Weapon.h"
#include "rts/Renderer.h"

namespace rts {

Actor::Actor(id_t id, const std::string &name, const Json::Value &params,
             bool targetable, bool collidable)
  : GameEntity(id, name, params, targetable, collidable),
    melee_timer_(0.f),
    meleeWeapon_(nullptr),
    rangedWeapon_(nullptr) {

  if (hasParam("meleeWeapon")) {
    meleeWeapon_ = new Weapon(strParam("meleeWeapon"), this);
  }
  if (hasParam("rangedWeapon")) {
    rangedWeapon_ = new Weapon(strParam("rangedWeapon"), this);
  }

  health_ = getMaxHealth();

  setMeshName(strParam("model"));
  setScale(glm::vec3(fltParam("modelSize")));
  resetTexture();

  if (hasParam("effects")) {
    auto effects = getParam("effects");
    for (int i = 0; i < effects.size(); i++) {
      const std::string effect_name = effects[i].asString();
      effects_[effect_name] = createEffect(effect_name);
    }
  }

  if (hasParam("actions")) {
    auto actions = getParam("actions");
    for (int i = 0; i < actions.size(); i++) {
      actions_.emplace_back(getID(), actions[i]);
    }
  }
}

Actor::~Actor() {
}

bool Actor::hasProperty(uint32_t property) const {
  if (property == P_ACTOR) {
    return true;
  }
  return GameEntity::hasProperty(property);
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
  if (msg["type"] == MessageTypes::ATTACK) {
    invariant(msg.isMember("pid"), "malformed attack message");
    invariant(msg.isMember("damage"), "malformed attack message");
    invariant(msg.isMember("damage_type"), "malformed attack message");
    
    // For graphics purpose, move if possible
    setTookDamage();

    // TODO(zack) figure out how to deal with this case (friendly fire)
    // when we have from, we can work that in here too
    invariant(toID(msg["pid"]) != getPlayerID(), "no friendly fire");

    // Just take damage for now
    health_ -= msg["damage"].asFloat();
    if (health_ <= 0.f) {
      Game::get()->destroyEntity(getID());
    }

    // If melee then we have to not melee
    if (msg["damage_type"] == "melee") {
      melee_timer_ = ::fltParam("global.meleeCooldown");
    }
  } else if (msg["type"] == MessageTypes::ORDER) {
    handleOrder(msg);
  } else if (msg["type"] == MessageTypes::ADD_STAT) {
    if (msg.isMember("healing")) {
      health_ += msg["healing"].asFloat();
      health_ = std::min(health_, getMaxHealth());
    }
  } else {
    GameEntity::handleMessage(msg);
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

  // TODO(zack): dirty hack :-(
  resetTexture();

  melee_timer_ -= dt;

  for (auto it = effects_.begin(); it != effects_.end();) {
    if (!it->second(this, dt)) {
      it = effects_.erase(it);
    } else {
      it++;
    }
  }
}

float Actor::distanceToEntity(const GameEntity *e) const {
  return rayBox2Intersection(
    getPosition2(),
    glm::normalize(e->getPosition2() - getPosition2()),
    e->getRect());
}
};  // rts
