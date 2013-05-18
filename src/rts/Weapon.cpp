#include "rts/Weapon.h"
#include "common/ParamReader.h"
#include "rts/Actor.h"
#include "rts/GameEntity.h"
#include "rts/MessageHub.h"
#include "rts/Projectile.h"

namespace rts {

Weapon::Weapon(const std::string &name, const Actor *owner)
    : name_(name),
    owner_(owner),
    state_(READY),
    t_(0.f),
    target_(NO_ENTITY) {
  if (strParam("type") == "ranged") {
    invariant(hasParam("projectile"), "ranged weapon missing projectile");
  }
}

float Weapon::getMaxRange() const {
  return param("range");
}

float Weapon::getArc() const {
  return param("arc");
}

Weapon::WeaponState Weapon::getState() const {
  return state_;
}

void Weapon::update(float dt) {
  t_ -= dt;

  // See transitions in description of WeaponState
  // Use += here to try nd compensate for a large dt
  switch (state_) {
    case WINDUP:
      if (t_ <= 0.f) {
        sendMessage();
        target_ = NO_ENTITY;
        state_ = WINDDOWN;
        t_ += param("winddown");
      }
      // fallthrough
    case WINDDOWN:
      if (t_ <= 0.f) {
        state_ = COOLDOWN;
        t_ += param("cooldown");
      }
      // fallthrough
    case COOLDOWN:
      if (t_ <= 0.f) {
        state_ = READY;
      }
      // fallthrough
    case READY:
      break;
    default:
      LOG(WARNING) << "Unknown weapon state " << state_ << '\n';
      break;
  }
}

void Weapon::sendMessage() {
  if (target_ == NO_ENTITY) {
    return;
  }
  std::string type = strParam("type");
  invariant(type == "ranged" || type == "melee", "invalid weapon type");

  if (type == "ranged") {
    // Spawn a projectile
    Json::Value params;
    params["entity_pid"] = toJson(owner_->getPlayerID());
    params["entity_pos"] = toJson(owner_->getPosition2());
    params["projectile_target"] = toJson(target_);
    params["projectile_owner"] = toJson(owner_->getID());
    Game::get()->spawnEntity(strParam("projectile"), params);
  } else if (type == "melee") {
    Message msg;
    msg["to"] = toJson(target_);
    msg["from"] = toJson(owner_->getID());
    msg["type"] = MessageTypes::ATTACK;
    msg["pid"] = toJson(owner_->getPlayerID());
    msg["damage"] = param("damage");
    msg["damage_type"] = type;
    MessageHub::get()->sendMessage(msg);
  }
}

void Weapon::fire(const GameEntity *target) {
  state_ = WINDUP;
  t_ = param("windup");
  target_ = target->getID();
}

void Weapon::interrupt() {
  target_ = NO_ENTITY;
}

bool Weapon::canAttack(const GameEntity *target) const {
  glm::vec2 delta = target->getPosition2() - owner_->getPosition2();
  float targetAngle = rad2deg(atan2(delta.y , delta.x));
  // difference between facing and target
  float arc = addAngles(targetAngle, -owner_->getAngle());
  float dist = owner_->distanceToEntity(target);

  // Need to be in range and in arc
  return (dist < getMaxRange() && fabs(arc) < getArc() / 2.f);
}

float Weapon::param(const std::string &p) const {
  return fltParam(name_ + "." + p);
}

std::string Weapon::strParam(const std::string &p) const {
  return ::strParam(name_ + "." + p);
}

bool Weapon::hasParam(const std::string &p) const {
  return ::hasParam(name_ + "." + p);
}
}  // rts
