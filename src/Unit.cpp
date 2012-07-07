#include "Unit.h"
#include "ParamReader.h"

Unit::Unit(int64_t playerID) :
    Entity(playerID),
    state_(new NullState(this))
{
    pos_ = glm::vec3(0, 0.5f, 0);
    dir_ = glm::vec2(0, 1);
    radius_ = 0.5f;

    vel_ = glm::vec3(0.f);
    maxSpeed_ = getParam("unit.maxSpeed");
}

void Unit::update(float dt)
{
    state_->update(dt);
}

bool Unit::needsRemoval() const
{
    return false;
}

void Unit::serialize(Json::Value &obj) const
{
    // TODO
}

MoveState::MoveState(const glm::vec3 &target, Unit *unit) :
    UnitState(unit),
    target_(target)
{
    // Make sure the unit stands on the terrain
    target_.y += unit_->getRadius(); 
    unit_->vel_ = unit_->maxSpeed_ * (target_ - unit_->getPosition());
}

void MoveState::update(float dt)
{
}

UnitState * MoveState::next()
{
    return NULL;
}
