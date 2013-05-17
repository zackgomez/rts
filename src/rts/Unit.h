#ifndef SRC_RTS_UNIT_H_
#define SRC_RTS_UNIT_H_

#include "rts/Actor.h"
#include "common/Logger.h"

namespace rts {

class UnitState;
class Building;

class Unit : public Actor {
 public:
  explicit Unit(id_t id, const std::string &name, const Json::Value &params);
  virtual ~Unit();

  virtual void handleMessage(const Message &msg);
  virtual void update(float dt);

  // Unit state functions
  // if the target is in range and within firing arc
  bool canAttack(const GameEntity *target) const;
  // if the unit is in range
  bool canCapture(const Building *target) const;
  // If this the target is within firing range
  bool withinRange(const GameEntity *target) const;
  // Attacks target if possible (within range, arc, cd available)
  void attackTarget(const GameEntity *target);
  // Captures target
  void captureTarget(const Building *target, float cap);

  const GameEntity *getTarget(id_t lastTargetID) const;
  glm::vec3 getTargetPos() const;

 protected:
  virtual void handleOrder(const Message &order);

  Weapon *weapon_;
  UnitState *state_;
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
  explicit MoveState(const glm::vec2 &target, Unit *unit);
  explicit MoveState(id_t targetID, Unit *unit);
  virtual ~MoveState() { }

  virtual void update(float dt);
  virtual UnitState * stop(UnitState *next);
  virtual UnitState * next();

 protected:
  void updateTarget();

  id_t targetID_;
  glm::vec2 target_;
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
  explicit AttackMoveState(const glm::vec2 &target, Unit *unit);
  virtual ~AttackMoveState();

  virtual void update(float dt);
  virtual UnitState * stop(UnitState *next);
  virtual UnitState * next();

 protected:
  glm::vec2 targetPos_;
  id_t targetID_;
};

class CaptureState : public UnitState {
 public:
  explicit CaptureState(id_t target_id, Unit *unit);
  virtual ~CaptureState();

  virtual void update(float dt);
  virtual UnitState *stop(UnitState *next);
  virtual UnitState * next();

 protected:
  id_t targetID_;
};
};

#endif  // SRC_RTS_UNIT_H_
