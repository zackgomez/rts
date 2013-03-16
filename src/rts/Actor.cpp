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
  setScale(glm::vec3(param("modelSize")));
  resetTexture();
}

Actor::~Actor() {
}

void Actor::resetTexture() {
  const Player *player = Game::get()->getPlayer(getPlayerID());
  auto color = player ? player->getColor() : vec3Param("global.defaultColor");
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
      melee_timer_ = fltParam("global.meleeCooldown");
    }
  } else if (msg["type"] == MessageTypes::ORDER) {
    handleOrder(msg);
  } else {
    GameEntity::handleMessage(msg);
  }
}
void Actor::handleOrder(const Message &order) {
  invariant(order["type"] == MessageTypes::ORDER, "unknown message type");
  invariant(order.isMember("order_type"), "missing order type");
  if (order["order_type"] == OrderTypes::ENQUEUE) {
    enqueue(order);
  } else {
    LOG(WARNING) << "Actor got unknown order: "
      << order.toStyledString() << '\n';
  }
}

void Actor::enqueue(const Message &queue_order) {
  invariant(queue_order.isMember("prod"), "missing production type");
  std::string prod_name = queue_order["prod"].asString();

  // TODO(zack) confirm that prod_name is something this Actor can produce

  Production prod;
  prod.max_time = fltParam(prod_name + ".cost.time");
  prod.time = prod.max_time;
  prod.name = prod_name;
  production_queue_.push(prod);
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
    Unit::TYPE,
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
};  // rts
