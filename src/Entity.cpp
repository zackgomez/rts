#include "Entity.h"

uint64_t Entity::lastID_ = 1;

Entity::Entity()
: playerID_(NO_PLAYER)
, id_(lastID_++)
{
}

Entity::Entity(int64_t playerID)
: playerID_(playerID)
, id_(lastID_++)
{
}

Entity::~Entity()
{
}

