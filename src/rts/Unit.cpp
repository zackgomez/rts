#define GLM_SWIZZLE_XYZW
#include "rts/Unit.h"
#include <cmath>
#include "common/ParamReader.h"
#include "rts/Game.h"
#include "rts/MessageHub.h"
#include "rts/Projectile.h"
#include "rts/Renderer.h"
#include "rts/Weapon.h"

namespace rts {

Unit::Unit(id_t id, const std::string &name, const Json::Value &params)
  : Actor(id, name, params, true),
    weapon_(nullptr),
    state_(new IdleState(this)) {

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
  Actor::update(dt);

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
}

bool Unit::canAttack(const GameEntity *e) const {
  if (e->hasProperty(GameEntity::P_CAPPABLE)) {
    return false;
  } else {
    return weapon_->canAttack(e);
  }
}

bool Unit::canCapture(const GameEntity *e) const {
  float dist = distanceToEntity(e);
  return e->hasProperty(GameEntity::P_CAPPABLE)
    && e->getPlayerID() != getPlayerID()
    && dist <= ::fltParam("global.captureRange");
}

bool Unit::withinRange(const GameEntity *e) const {
  glm::vec2 targetPos = e->getPosition2();
  float dist = glm::distance(getPosition2(), targetPos);
  return dist < weapon_->getMaxRange();
}

void Unit::attackTarget(const GameEntity *e) {
  assert(e);

  // If we can't attack, don't!
  // TODO(zack) I know this is inefficient (the caller probably just called
  // canAttack(), but it's alright for now)
  if (weapon_->getState() != Weapon::READY || !canAttack(e)) {
    return;
  }

  weapon_->fire(e);
}

void Unit::captureTarget(const GameEntity *e, float cap) {
  assert(e);
  invariant(canCapture(e), "should be able to cap");

  // TODO(zack): MessageHub::sendCaptureMessage
  Message msg;
  msg["to"] = toJson(e->getID());
  msg["from"] = toJson(getID());
  msg["type"] = MessageTypes::CAPTURE;
  msg["pid"] = toJson(getPlayerID());
  msg["cap"] = cap;

  MessageHub::get()->sendMessage(msg);
}

const GameEntity * Unit::getTarget(id_t lastTargetID) const {
  const GameEntity *target = nullptr;
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
    target = Renderer::get()->findEntity(
      [&](const GameEntity *e) -> float {
        if (e->getPlayerID() != NO_PLAYER
            && e->getTeamID() != getTeamID()
            && e->hasProperty(P_TARGETABLE)
            && !e->hasProperty(P_CAPPABLE)) {
          float dist = distanceToEntity(e);
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
  const GameEntity *target = unit_->getTarget(targetID_);
  if (target) {
    // If target is in range, attack
    if (unit_->canAttack(target)) {
      unit_->attackTarget(target);
    } else {
      // otherwse rotate towards
      unit_->turnTowards(target->getPosition2(), dt);
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
    const GameEntity *e = Game::get()->getEntity(targetID_);
    if (!e) {
      targetID_ = NO_ENTITY;
    } else {
      target_ = e->getPosition2();
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
  const GameEntity *target = Game::get()->getEntity(targetID_);
  // TODO(brooklyn) unit out of global sight (must be accounted for)
  if (!target)
    return;

  // If we can shoot them, do so
  if (unit_->canAttack(target)) {
    unit_->attackTarget(target);
  } else {
    // Otherwise move towards target
    unit_->moveTowards(target->getPosition2(), dt);
  }
}

UnitState * AttackState::next() {
  // We're done pursuing when the target is DEAD
  const GameEntity *target = Game::get()->getEntity(targetID_);

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
  const GameEntity *targetEnt = unit_->getTarget(targetID_);
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
      unit_->moveTowards(targetEnt->getPosition2(), dt);
    }
  }

  // Store targetEnt
  targetID_ = targetEnt ? targetEnt->getID() : NO_ENTITY;
}

UnitState * AttackMoveState::next() {
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
  invariant(
      Game::get()->getEntity(targetID_)->hasProperty(GameEntity::P_CAPPABLE),
      "capture target must be cappable");
}

CaptureState::~CaptureState() {
}

void CaptureState::update(float dt) {
  // Default to no movement
  unit_->remainStationary();
  const GameEntity *target = Game::get()->getEntity(targetID_);
  if (!target) {
    return;
  }
  invariant(target->hasProperty(GameEntity::P_CAPPABLE),
      "capture target must be a building");

  // If we can cap, do so.
  if (unit_->canCapture(target)) {
    unit_->captureTarget(target, dt);
  } else {
    // Otherwise move towards target
    unit_->moveTowards(target->getPosition2(), dt);
  }
}

UnitState * CaptureState::next() {
  // We're done capping when the building belongs to us.
  const GameEntity *target = Game::get()->getEntity(targetID_);
  if (target->getTeamID() == unit_->getTeamID()) {
    return new IdleState(unit_);
  }

  return nullptr;
}

UnitState * CaptureState::stop(UnitState *next) {
  return next;
}
};  // rts
