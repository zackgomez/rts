#define GLM_SWIZZLE_XYZW
#include "rts/Unit.h"
#include <cmath>
#include "common/ParamReader.h"
#include "rts/Building.h"
#include "rts/Game.h"
#include "rts/MessageHub.h"
#include "rts/Projectile.h"
#include "rts/Weapon.h"

namespace rts {

LoggerPtr Unit::logger_;
const std::string Unit::TYPE = "UNIT";

Unit::Unit(const std::string &name, const Json::Value &params)
  : Actor(name, params, true),
    weapon_(nullptr),
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
  UnitState *next = nullptr;
  if (order["order_type"] == OrderTypes::MOVE) {
    // TODO(zack) add following a unit here
    next = new MoveState(toVec2(order["target"]), this);
  } else if (order["order_type"] == OrderTypes::ATTACK) {
    invariant(order.isMember("enemy_id") || order.isMember("target"),
              "attack order missing target: " + order.toStyledString());
    if (order.isMember("enemy_id")) {
      next = new AttackState(toID(order["enemy_id"]), this);
    } else if (order.isMember("target")) {
      next = new AttackMoveState(toVec2(order["target"]), this);
    }
  } else if (order["order_type"] == OrderTypes::CAPTURE) {
    invariant(order.isMember("enemy_id"), "capture order missing target: " +
        order.toStyledString());
    next = new CaptureState(toID(order["enemy_id"]), this);
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
  if (e->getType() == Building::TYPE) {
    return !((Building*)e)->canCapture(getID()) && weapon_->canAttack(e);
  } else {
    return weapon_->canAttack(e);
  }
}

bool Unit::canCapture(const Building *e) const {
  float dist = glm::distance(e->getPosition(), getPosition());
  return e->canCapture(getID())
      && dist <= fltParam("global.captureRange");
}

bool Unit::withinRange(const Entity *e) const {
  glm::vec2 targetPos = e->getPosition();
  float dist = glm::distance(pos_, targetPos);
  return dist < weapon_->getMaxRange();
}

void Unit::turnTowards(const glm::vec2 &targetPos, float dt) {
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

void Unit::moveTowards(const glm::vec2 &targetPos, float dt) {
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

void Unit::captureTarget(const Building *e, float cap) {
  assert(e);

  invariant(e->canCapture(getID()), "should be able to cap");

  // TODO(zack): MessageHub::sendCaptureMessage
  Message msg;
  msg["to"] = toJson(e->getID());
  msg["from"] = toJson(getID());
  msg["type"] = MessageTypes::CAPTURE;
  msg["pid"] = toJson(getPlayerID());
  msg["cap"] = cap;

  MessageHub::get()->sendMessage(msg);
}

const Entity * Unit::getTarget(id_t lastTargetID) const {
  const Entity *target = nullptr;
  // Default to last target
  if (lastTargetID != NO_ENTITY
      && (target = Game::get()->getEntity(lastTargetID))) {
    // Can't attack them if they're too far away
    if (!withinRange(target)) {
      target = nullptr;
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
            && e->getTeamID() != getTeamID()
            && e->isTargetable()) {
          float dist = glm::distance(
            pos_,
            e->getPosition());
          // Only take ones that are within attack range, sort
          // by distance
          return dist < getSight() ? dist : HUGE_VAL;
        }
        return HUGE_VAL;
      });
  }

  return target;
}

IdleState::IdleState(Unit *unit)
  : UnitState(unit),
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
    } else {
      // otherwse rotate towards
      unit_->turnTowards(target->getPosition(), dt);
    }
  }

  targetID_ = target ? target->getID() : NO_ENTITY;
}

UnitState * IdleState::next() {
  return nullptr;
}

UnitState * IdleState::stop(UnitState *next) {
  return next;
}

MoveState::MoveState(const glm::vec2 &target, Unit *unit)
  : UnitState(unit),
    targetID_(NO_ENTITY),
    target_(target) {
}

MoveState::MoveState(id_t targetID, Unit *unit)
  : UnitState(unit),
    targetID_(targetID),
    target_(0.f) {
}

void MoveState::updateTarget() {
  if (targetID_ != NO_ENTITY) {
    const Entity *e = Game::get()->getEntity(targetID_);
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
  glm::vec2 pos = unit_->Entity::getPosition();
  glm::vec2 target = target_;

  // If we've reached destination point
  if (targetID_ == NO_ENTITY && unit_->pointInEntity(target)) {
    return new IdleState(unit_);
  }

  // Or target entity has died
  if (targetID_ != NO_ENTITY
      && Game::get()->getEntity(targetID_) == nullptr) {
    return new IdleState(unit_);
  }

  // Keep on movin'
  return nullptr;
}


AttackState::AttackState(id_t targetID, Unit *unit)
  : UnitState(unit),
    targetID_(targetID) {
}

AttackState::~AttackState() {
}

void AttackState::update(float dt) {
  // Default to no movement
  unit_->remainStationary();
  const Entity *target = Game::get()->getEntity(targetID_);
  // TODO(brooklyn) unit out of global sight (must be accounted for)
  if (!target)
    return;

  // If we can shoot them, do so
  if (unit_->canAttack(target)) {
    unit_->attackTarget(target);
  } else {
    // Otherwise move towards target
    unit_->moveTowards(target->getPosition(), dt);
  }
}

UnitState * AttackState::next() {
  // We're done pursuing when the target is DEAD
  const Entity *target = Game::get()->getEntity(targetID_);

  // Checks that distance between units is in sight range
  // TODO(brooklyn) add condition to consider global sight
  if (!target)
    return new IdleState(unit_);

  return nullptr;
}

UnitState * AttackState::stop(UnitState *next) {
  return next;
}

AttackMoveState::AttackMoveState(const glm::vec2 &target, Unit *unit)
  : UnitState(unit),
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
  } else {
    // otherwise pursue target
    // no movement by default
    unit_->remainStationary();

    // If we can shoot them, do so
    if (unit_->canAttack(targetEnt)) {
      unit_->attackTarget(targetEnt);
    } else {
      // Otherwise move towards target
      unit_->moveTowards(targetEnt->getPosition(), dt);
    }
  }

  // Store targetEnt
  targetID_ = targetEnt ? targetEnt->getID() : NO_ENTITY;
}

UnitState * AttackMoveState::next() {
  glm::vec2 pos = unit_->Entity::getPosition().xy;
  glm::vec2 target = targetPos_.xy;
  // If we've reached destination point
  if (unit_->pointInEntity(target)) {
    return new IdleState(unit_);
  }

  return nullptr;
}

UnitState * AttackMoveState::stop(UnitState *next) {
  return next;
}

CaptureState::CaptureState(id_t targetID, Unit *unit)
  : UnitState(unit),
    targetID_(targetID) {
  invariant(Game::get()->getEntity(targetID_)->getType() == Building::TYPE,
            "capture target must be a building");
}

CaptureState::~CaptureState() {
}

void CaptureState::update(float dt) {
  // Default to no movement
  unit_->remainStationary();
  const Entity *target = Game::get()->getEntity(targetID_);
  if (!target) {
    return;
  }
  invariant(target->getType() == Building::TYPE,
      "capture target must be a building");
  const Building *btarget = (const Building *) target;

  // If we can cap, do so.
  if (unit_->canCapture(btarget)) {
    unit_->captureTarget(btarget, dt);
  } else {
    // Otherwise move towards target
    unit_->moveTowards(btarget->getPosition(), dt);
  }
}

UnitState * CaptureState::next() {
  // We're done capping when the building belongs to us.
  const Entity *target = Game::get()->getEntity(targetID_);
  if (target->getTeamID() == unit_->getTeamID()) {
    return new IdleState(unit_);
  }

  return nullptr;
}

UnitState * CaptureState::stop(UnitState *next) {
  return next;
}
};  // rts
