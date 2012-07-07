#pragma once
#include "Entity.h"
#include <glm/glm.hpp>

class UnitState;
class MoveState;

class Unit :
    public Entity
{
public:
    explicit Unit(int64_t playerID);
    virtual ~Unit() { }

    virtual std::string getType() const { return "Unit"; }

    virtual void handleMessage(const Message &msg);
    virtual void update(float dt);
    virtual bool needsRemoval() const;

protected:
    virtual void serialize(Json::Value &obj) const;

    UnitState *state_;

    float maxSpeed_;
    glm::vec3 vel_;

    friend class MoveState;
};



namespace OrderTypes
{
    const std::string MOVE = "MOVE";
    const std::string STOP = "STOP";
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

    //virtual void serialize(Json::Value &obj) = 0;

protected:
    Unit *unit_;

    friend class MoveState;
};

class NullState : public UnitState
{
public:
    explicit NullState(Unit *unit) : UnitState(unit) { }
    virtual ~NullState() { }

    virtual void update(float dt) { }
    virtual void stop() { }
    virtual UnitState * next() { return NULL; }
};

class MoveState : public UnitState
{
public:
    explicit MoveState(const glm::vec3 &target, Unit *unit);
    virtual ~MoveState() { }

    virtual void update(float dt);
    virtual void stop() { }
    virtual UnitState * next();

protected:
    glm::vec3 target_;
    glm::vec3 prev_target_;
};

