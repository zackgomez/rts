#define GLM_SWIZZLE_XYZW
#include <cmath>
#include "Unit.h"
#include "ParamReader.h"
#include "MessageHub.h"
#include "Projectile.h"
#include "Weapon.h"

namespace rts {

LoggerPtr Unit::logger_;
const std::string Unit::TYPE = "UNIT";

Unit::Unit(const std::string &name, const Json::Value &params) :
  Actor(name, params, true),
  weapon_(NULL),
  state_(new IdleState(this)) {

  if (!logger_.get()) {
    logger_ = Logger::getLogger("Unit");
  }

  // these are inited in Actor
  weapon_ = rangedWeapon_ ? rangedWeapon_ : meleeWeapon_;
}

Unit::~Unit() {
  delete rangedWeapon_;
  delete meleeWeapon_;
}

void Unit::handleMessage(const Message &msg) {
  if (msg["type"] == MessageTypes::ORDER) {
    handleOrder(msg);
  } else {
    Actor::handleMessage(msg);
  }
}

void Unit::handleOrder(const Message &order) {
  invariant(order["type"] == MessageTypes::ORDER, "unexpected message type");
  invariant(order.isMember("order_type"), "malformed order message");
  UnitState *next = NULL;
  if (order["order_type"] == OrderTypes::MOVE) {
    // TODO(zack) add following a unit here
    next = new MoveState(toVec3(order["target"]), this);
  } else if (order["order_type"] == OrderTypes::ATTACK) {
    invariant(order.isMember("enemy_id") || order.isMember("target"),
              "attack order missing target: " + order.toStyledString());
    if (order.isMember("enemy_id")) {
      next = new AttackState(toID(order["enemy_id"]), this);
    } else if (order.isMember("target")) {
      next = new AttackMoveState(toVec3(order["target"]), this);
    }
  } else if (order["order_type"] == OrderTypes::STOP) {
    next = new IdleState(this);
  } else {
    Actor::handleOrder(order);
  }

  // If a state transition, tell the current state we're transitioning
  if (next) {
    next = state_->stop(next);
    // The state might have changed, or rejected the next state
    if (next) {
      delete state_;
      state_ = next;
    }
  }
}

void Unit::update(float dt) {
  if (melee_timer_ <= 0.f) {
    weapon_ = rangedWeapon_ ? rangedWeapon_ : meleeWeapon_;
  } else {
    weapon_ = meleeWeapon_;
  }

  state_->update(dt);

  UnitState *next;
  if ((next = state_->next())) {
    delete state_;
    state_ = next;
  }

  // count down the attack timer
  if (weapon_) {
    weapon_->update(dt);
  }

  Actor::update(dt);
}

bool Unit::canAttack(const Entity *e) const {
  return weapon_->canAttack(e);
}

bool Unit::withinRange(const Entity *e) const {
  glm::vec3 targetPos = e->getPosition();
  float dist = glm::distance(pos_, targetPos);
  return dist < weapon_->getMaxRange();
}

void Unit::turnTowards(const glm::vec3 &targetPos, float dt) {
  float desired_angle = angleToTarget(targetPos);
  float delAngle = addAngles(desired_angle, -angle_);
  float turnRate = param("turnRate");
  // rotate
  // only rotate when not close enough
  // Would overshoot, just move directly there
  if (fabs(delAngle) < turnRate * dt) {
    turnSpeed_ = delAngle / dt;
  } else {
    turnSpeed_ = glm::sign(delAngle) * turnRate;
  }
  // No movement
  speed_ = 0.f;
}

void Unit::moveTowards(const glm::vec3 &targetPos, float dt) {
  float dist = glm::length(targetPos - pos_);
  float speed = param("speed");
  // rotate
  turnTowards(targetPos, dt);
  // move
  // Set speed careful not to overshoot
  if (dist < speed * dt) {
    speed = dist / dt;
  }
  speed_ = speed;
}

void Unit::remainStationary() {
  speed_ = 0.f;
  turnSpeed_ = 0.f;
}

void Unit::attackTarget(const Entity *e) {
  assert(e);

  // If we can't attack, don't!
  // TODO(zack) I know this is inefficient (the caller probably just called
  // canAttack(), but it's alright for now)
  if (weapon_->getState() != Weapon::READY || !canAttack(e)) {
    return;
  }

  weapon_->fire(e);
}

const Entity * Unit::getTarget(id_t lastTargetID) const {
  const Entity *target = NULL;
  // Default to last target
  if (lastTargetID != NO_ENTITY
      && (target = MessageHub::get()->getEntity(lastTargetID))) {
    // Can't attack them if they're too far away
    if (!withinRange(target)) {
      target = NULL;
    }
  }
  // Last target doesn't exist or isn't viable, find a new one
  if (!target) {
    // Explanation: this creates a lambda function, the [&] is so it captures
    // this, the arg is a const Entity *, and it returns a float
    // In vim, these {} are marked as errors, that's just because vim doesn't
    // know c++11
    target = MessageHub::get()->findEntity(
    [&](const Entity *e) -> float {
      if (e->getPlayerID() != NO_PLAYER
      && e->getPlayerID() != getPlayerID()
      && e->isTargetable()) {
        float dist = glm::distance(
          pos_,
          e->getPosition());
        // Only take ones that are within attack range, sort
        // by distance
        return dist < weapon_->getMaxRange() ? dist : HUGE_VAL;
      }
      return HUGE_VAL;
    }
             );
  }

  return target;
}

IdleState::IdleState(Unit *unit) :
  UnitState(unit),
  targetID_(NO_ENTITY) {
}

void IdleState::update(float dt) {
  // Default to no movement
  unit_->remainStationary();

  // If an enemy is within our firing range, then fuck him up
  const Entity *target = unit_->getTarget(targetID_);
  if (target) {
    // If target is in range, attack
    if (unit_->canAttack(target)) {
      unit_->attackTarget(target);
    }
    // otherwse rotate towards
    else {
      unit_->turnTowards(target->getPosition(), dt);
    }
  }

  targetID_ = target ? target->getID() : NO_ENTITY;
}

UnitState * IdleState::next() {
  return NULL;
}

UnitState * IdleState::stop(UnitState *next) {
  return next;
}

MoveState::MoveState(const glm::vec3 &target, Unit *unit) :
  UnitState(unit),
  targetID_(NO_ENTITY),
  target_(target) {
}

MoveState::MoveState(id_t targetID, Unit *unit) :
  UnitState(unit),
  targetID_(targetID),
  target_(0.f) {
}

void MoveState::updateTarget() {
  if (targetID_ != NO_ENTITY) {
    const Entity *e = MessageHub::get()->getEntity(targetID_);
    if (!e) {
      targetID_ = NO_ENTITY;
    } else {
      target_ = e->getPosition();
    }
  }
}

void MoveState::update(float dt) {
  // Calculate some useful values
  updateTarget();
  unit_->moveTowards(target_, dt);
}

UnitState * MoveState::stop(UnitState *next) {
  return next;
}

UnitState * MoveState::next() {
  glm::vec2 pos = unit_->Entity::getPosition().xz;
  glm::vec2 target = target_.xz;
  float dist = glm::distance(target, pos);

  // If we've reached destination point
  if (targetID_ == NO_ENTITY && dist < unit_->getRadius() / 2.f) {
    return new IdleState(unit_);
  }

  // Or target entity has died
  if (targetID_ != NO_ENTITY
      && MessageHub::get()->getEntity(targetID_) == NULL) {
    return new IdleState(unit_);
  }

  // Keep on movin'
  return NULL;
}


AttackState::AttackState(id_t targetID, Unit *unit) :
  UnitState(unit),
  targetID_(targetID) {
}

AttackState::~AttackState() {
}

void AttackState::update(float dt) {
  // Default to no movement
  unit_->remainStationary();
  const Entity *target = MessageHub::get()->getEntity(targetID_);
  // TODO(zack) or target is out of sight
  if (!target) {
    return;
  }

  // If we can shoot them, do so
  if (unit_->canAttack(target)) {
    unit_->attackTarget(target);
  }
  // Otherwise move towards target
  else {
    unit_->moveTowards(target->getPosition(), dt);
  }
}

UnitState * AttackState::next() {
  // We're done pursuing when the target is DEAD
  // TODO(zack): incorporate a sight radius
  const Entity *target = MessageHub::get()->getEntity(targetID_);
  if (!target) {
    return new IdleState(unit_);
  }

  return NULL;
}

UnitState * AttackState::stop(UnitState *next) {
  return next;
}

AttackMoveState::AttackMoveState(const glm::vec3 &target, Unit *unit) :
  UnitState(unit),
  targetPos_(target),
  targetID_(NO_ENTITY) {
}

AttackMoveState::~AttackMoveState() {
}

void AttackMoveState::update(float dt) {
  const Entity *targetEnt = unit_->getTarget(targetID_);
  // If no target, move towards location
  if (!targetEnt) {
    unit_->moveTowards(targetPos_, dt);
  }
  // otherwise pursue target
  else {
    // no movement by default
    unit_->remainStationary();
    // If we can shoot them, do so
    if (unit_->canAttack(targetEnt)) {
      unit_->attackTarget(targetEnt);
    }
    // Otherwise move towards target
    else {
      unit_->moveTowards(targetEnt->getPosition(), dt);
    }
  }

  // Store targetEnt
  targetID_ = targetEnt ? targetEnt->getID() : NO_ENTITY;
}

UnitState * AttackMoveState::next() {
  glm::vec2 pos = unit_->Entity::getPosition().xz;
  glm::vec2 target = targetPos_.xz;
  // If we've reached destination point
  float dist = glm::distance(target, pos);
  if (dist < unit_->getRadius() / 2.f) {
    return new IdleState(unit_);
  }

  return NULL;
}

UnitState * AttackMoveState::stop(UnitState *next) {
  return next;
}
}; // rts
