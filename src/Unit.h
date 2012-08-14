#pragma once
#include <glm/glm.hpp>
#include "Actor.h"
#include "Logger.h"

namespace rts {

class UnitState;

class Unit : public Actor {
public:
  explicit Unit(const std::string &name, const Json::Value &params);
  virtual ~Unit();

  static const std::string TYPE;
  virtual const std::string getType() const {
    return TYPE;
  }

  virtual float getSpeed() const {
    return speed_;
  }
  virtual float getTurnSpeed() const {
    return turnSpeed_;
  }

  virtual void handleMessage(const Message &msg);
  virtual void update(float dt);

  // Unit state functions
  // if the target is in range and within firing arc
  bool canAttack(const Entity *target) const;
  // If this the target is within firing range
  bool withinRange(const Entity *target) const;
  // Rotates to face position
  void turnTowards(const glm::vec3 &pos, float dt);
  // Moves towards position as fast as possible (probably rotates)
  void moveTowards(const glm::vec3 &pos, float dt);
  // Don't move or rotate
  void remainStationary();
  // Attacks target if possible (within range, arc, cd available)
  void attackTarget(const Entity *target);

  const Entity *getTarget(id_t lastTargetID) const;

protected:
  virtual void handleOrder(const Message &order);

  Weapon *weapon_;
  UnitState *state_;

private:
  static LoggerPtr logger_;

};

class UnitState {
public:
  explicit UnitState(Unit *unit) : unit_(unit) { }
  virtual ~UnitState() { }

  virtual void update(float dt) = 0;
  // Called when this state is interrupted, return is same as next
  virtual UnitState * stop(UnitState *next) = 0;
  // Called each frame, when returns non NULL, sets new state
  virtual UnitState * next() = 0;

protected:
  Unit *unit_;
};

class IdleState : public UnitState {
public:
  explicit IdleState(Unit *unit);
  virtual ~IdleState() { }

  virtual void update(float dt);
  virtual UnitState * stop(UnitState *next);
  virtual UnitState * next();

protected:
  // The last entity attacked, we prioritize attack them again
  id_t targetID_;
};

class MoveState : public UnitState {
public:
  explicit MoveState(const glm::vec3 &target, Unit *unit);
  explicit MoveState(id_t targetID, Unit *unit);
  virtual ~MoveState() { }

  virtual void update(float dt);
  virtual UnitState * stop(UnitState *next);
  virtual UnitState * next();

protected:
  void updateTarget();

  id_t targetID_;
  glm::vec3 target_;
};

class AttackState : public UnitState {
public:
  explicit AttackState(id_t target_id, Unit *unit);
  virtual ~AttackState();

  virtual void update(float dt);
  virtual UnitState * stop(UnitState *next);
  virtual UnitState * next();

protected:
  id_t targetID_;
};

class AttackMoveState : public UnitState {
public:
  explicit AttackMoveState(const glm::vec3 &target, Unit *unit);
  virtual ~AttackMoveState();

  virtual void update(float dt);
  virtual UnitState * stop(UnitState *next);
  virtual UnitState * next();

protected:
  glm::vec3 targetPos_;
  id_t targetID_;
};

};
