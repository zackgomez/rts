#include "Weapon.h"
#include "Entity.h"
#include "MessageHub.h"
#include "Projectile.h"
#include "ParamReader.h"

namespace rts {

Weapon::Weapon(const std::string &name, const Entity *owner) :
  name_(name),
  owner_(owner),
  state_(READY),
  t_(0.f) {
  if (strParam("type") == "ranged")
    assert(hasStrParam("projectile"));
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
      if (!msg_.isNull())
        MessageHub::get()->sendMessage(msg_);
      msg_ = Message();
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

void Weapon::fire(const Entity *target) {
  state_ = WINDUP;
  t_ = param("windup");

  // Can the message 
  // TODO(zack) replace with spawn entity function, can the params here then
  msg_.clear();
  std::string type = strParam("type");
  assert ((type == "ranged" || type == "melee") && "invalid weapon type");
  if (type == "ranged") {
    msg_["to"] = toJson(NO_ENTITY); // Send to game object
    msg_["type"] = MessageTypes::SPAWN_ENTITY;
    msg_["entity_class"] = Projectile::TYPE;
    msg_["entity_name"] = strParam("projectile");
    msg_["entity_pid"] = toJson(owner_->getPlayerID());
    msg_["entity_pos"] = toJson(owner_->getPosition());
    msg_["projectile_target"] = toJson(target->getID());
  } else if (strParam("type") == "melee") {
    msg_["to"] = toJson(target->getID());
    msg_["type"] = MessageTypes::ATTACK;
    msg_["pid"] = toJson(owner_->getPlayerID());
    msg_["damage"] = param("damage");
  } // else handled by assert
}

void Weapon::interrupt() {
  msg_ = Message();
}

bool Weapon::canAttack(const Entity *target) const {
  glm::vec3 targetPos = target->getPosition();
  glm::vec3 delta = targetPos - owner_->getPosition();
  float targetAngle = rad2deg(atan2(delta.z , delta.x));
  // difference between facing and target
  float arc = addAngles(targetAngle, -owner_->getAngle());
  float dist = glm::length(delta);

  // Need to be in range and in arc
  return (dist < getMaxRange() && fabs(arc) < getArc() / 2.f);
}

float Weapon::param(const std::string &p) const {
  return getParam(name_ + "." + p);
}
std::string Weapon::strParam(const std::string &p) const {
  return ::strParam(name_ + "." + p);
}

bool Weapon::hasParam(const std::string &p) const
{
  return ParamReader::get()->hasFloat(name_ + "." + p);
}

bool Weapon::hasStrParam(const std::string &p) const
{
  return ParamReader::get()->hasString(name_ + "." + p);
}

} // rts
