#include "Entity.h"

uint64_t Entity::lastID_ = 1;

Entity::Entity() :
    id_(lastID_++)
,   playerID_(NO_PLAYER)
{
}

Entity::Entity(int64_t playerID) :
    id_(lastID_++)
,   playerID_(playerID)
{
}

Entity::~Entity()
{
    for (auto &listener : listeners_)
        listener->removal();
}

void Entity::registerListener(EntityListener *listener)
{
    listeners_.insert(listener);
}

void Entity::removeListener(EntityListener *listener)
{
    auto it = listeners_.find(listener);
    if (it != listeners_.end())
        listeners_.erase(it);
}
