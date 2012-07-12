#include "Entity.h"

eid_t Entity::lastID_ = 1;

Entity::Entity()
: playerID_(NO_PLAYER)
, id_(lastID_++)
{
}

Entity::Entity(int64_t playerID, glm::vec3 pos)
: playerID_(playerID)
, id_(lastID_++)
, pos_(pos)
{
}

Entity::~Entity()
{
}

