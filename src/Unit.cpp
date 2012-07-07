#include "Unit.h"

Unit::Unit(int64_t playerID) :
    Entity(playerID)
{
    pos_ = glm::vec3(0, 0.5f, 0);
    dir_ = glm::vec2(0, 1);
    radius_ = 0.5f;
}

void Unit::update(float dt)
{
}

bool Unit::needsRemoval() const
{
    return false;
}

void Unit::serialize(Json::Value &obj) const
{
}
