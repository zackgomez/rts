#pragma once
#include "Actor.h"
#include "Mobile.h"
#include <glm/glm.hpp>
#include "Logger.h"

class UnitState;
class MoveState;

class Unit :
    public Mobile, public Actor
{
public:
    explicit Unit(int64_t playerID, const glm::vec3 &pos,
            const std::string &name);
    virtual ~Unit() { }

    virtual float getSpeed() const { return speed_; }
    virtual float getTurnSpeed() const { return turnSpeed_; }

    virtual void handleMessage(const Message &msg);
    virtual void update(float dt);
    virtual bool needsRemoval() const;

protected:
    UnitState *state_;
    void handleOrder(const Message &order);

private:
    static LoggerPtr logger_;

    friend class NullState;
    friend class MoveState;
    friend class AttackState;
};

class UnitState
{
public:
    explicit UnitState(Unit *unit) : unit_(unit) { }
    virtual ~UnitState() { } 

    virtual void update(float dt) = 0;
    // Called when this state is ended early
    virtual void stop() = 0;
    // Called each frame, when returns non NULL, sets new state
    virtual UnitState * next() = 0;

    virtual std::string getName() = 0;

protected:
    Unit *unit_;
};

class NullState : public UnitState
{
public:
    explicit NullState(Unit *unit) : UnitState(unit) { }
    virtual ~NullState() { }

    virtual void update(float dt);
    virtual void stop() { }
    virtual UnitState * next() { return NULL; }

    virtual std::string getName() { return "NullState"; }
};

class MoveState : public UnitState
{
public:
    explicit MoveState(const glm::vec3 &target, Unit *unit);
    virtual ~MoveState() { }

    virtual void update(float dt);
    virtual void stop();
    virtual UnitState * next();

    virtual std::string getName() { return "MoveState"; }

protected:
    glm::vec3 target_;
};

class AttackState: public UnitState
{
public:
    explicit AttackState(eid_t target_id, Unit *unit);
    virtual ~AttackState() { }

    virtual void update(float dt);
    virtual void stop();
    virtual UnitState * next();

    virtual std::string getName() { return "AttackState"; }
protected:
    eid_t target_id_;
};

