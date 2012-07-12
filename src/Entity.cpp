#include "Entity.h"

eid_t Entity::lastID_ = 1;

Entity::Entity()
: id_(lastID_++)
, playerID_(NO_PLAYER)
, pos_(0.f)
{
}

Entity::Entity(int64_t playerID, glm::vec3 pos)
: id_(lastID_++)
, playerID_(playerID)
, pos_(pos)
{
}

Entity::~Entity()
{
}

