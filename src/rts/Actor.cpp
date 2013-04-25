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
}

Actor::~Actor() {
}

bool Actor::hasProperty(uint32_t property) const {
  if (property == P_ACTOR) {
    return true;
  }
  return GameEntity::hasProperty(property);
}

Json::Value Actor::getActions() const {
  if (!hasParam("actions")) {
    return Json::Value();
  }
  return getParam("actions");
}

void Actor::resetTexture() {
  const Player *player = Game::get()->getPlayer(getPlayerID());
  auto color = player ? player->getColor() : ::vec3Param("global.defaultColor");
  GLuint texture = hasParam("texture")
    ? ResourceManager::get()->getTexture(strParam("texture"))
    : 0;
  setMaterial(createMaterial(color, 10.f, texture));
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
      MessageHub::get()->sendRemovalMessage(this);
    }

    // If melee then we have to not melee
    if (msg["damage_type"] == "melee") {
      melee_timer_ = ::fltParam("global.meleeCooldown");
    }
  } else if (msg["type"] == MessageTypes::ORDER) {
    handleOrder(msg);
  } else if (msg["type"] == MessageTypes::COLLISION) {
    auto* collider = Game::get()->getEntity(toID(msg["from"]));
    if (!collider->hasProperty(P_ACTOR)
        || collider->getPlayerID() != getPlayerID()) {
      return;
    }

    // if either entity is moving forward, just ignore the message
    if (getSpeed() != 0.f || collider->getSpeed() != 0.f) {
      return;
    }

    // get difference between positions at time of intersection
    float dt = msg["intersection_time"].asFloat();
    auto diff = getPosition2(dt) - collider->getPosition2(dt);
    float overlap_dist = glm::length(diff);

    // seperate away, or randomly if exactly the same pos
    auto dir = (overlap_dist > 1e-6f)
      ? diff / overlap_dist
      : randDir2();

    float bumpSpeed = ::fltParam("global.bumpSpeed");
    addBumpVel(glm::vec3(dir * bumpSpeed, 0.f));
  } else {
    GameEntity::handleMessage(msg);
  }
}
void Actor::handleOrder(const Message &order) {
  invariant(order["type"] == MessageTypes::ORDER, "unknown message type");
  invariant(order.isMember("order_type"), "missing order type");
  if (order["order_type"] == OrderTypes::ACTION) {
    auto actions = getActions();
    invariant(
        order.isMember("action_idx") && order["action_idx"].asInt() < actions.size(),
        "Bad action index");
    auto action = actions[order["action_idx"].asInt()];
    handleAction(action);
  } else {
    LOG(WARNING) << "Actor got unknown order: "
      << order.toStyledString() << '\n';
  }
}

void Actor::handleAction(const Json::Value &action) {
  invariant(action.isMember("type"), "missing action type");

  if (action["type"] == "production") {
    invariant(action.isMember("prod_name"), "missing production type");
    invariant(action.isMember("time_cost"), "missing production time cost");
    invariant(action.isMember("req_cost"), "missing production req cost");
    std::string prod_name = action["prod_name"].asString();
    float prod_time = action["time_cost"].asFloat();
    float req_cost = action["req_cost"].asFloat();

    // Enough resources?
    if (Game::get()->getResources(getPlayerID()).requisition < req_cost) {
      return;
    }

    // Pay for it
    MessageHub::get()->sendResourceMessage(
        getID(),
        getPlayerID(),
        ResourceTypes::REQUISITION,
        -req_cost);

    Production prod;
    prod.max_time = prod_time;
    prod.time = prod_time;
    prod.name = prod_name;
    production_queue_.push(prod);
  } else {
    invariant_violation(
        "unknown action type in message " + action.toStyledString());
  }
}

void Actor::produce(const std::string &prod_name) {
  // TODO(zack) generalize this to also include upgrades etc
  Json::Value params;
  params["entity_pid"] = toJson(getPlayerID());
  // TODO(zack) (make this a param)
  params["entity_pos"] = toJson(
      ModelEntity::getPosition2() + ModelEntity::getDirection());
  params["entity_angle"] = ModelEntity::getAngle();

  MessageHub::get()->sendSpawnMessage(
    getID(),
    prod_name,
    params);
}

void Actor::update(float dt) {
  GameEntity::update(dt);

  // TODO(zack): dirty hack :-(
  resetTexture();

  melee_timer_ -= dt;

  // Update production
  if (!production_queue_.empty()) {
    production_queue_.front().time -= dt;
    if (production_queue_.front().time <= 0) {
      produce(production_queue_.front().name);
      production_queue_.pop();
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
