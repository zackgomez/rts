#include "Targetable.h"

Targetable::Targetable() :
    health_max_(0),
    health_current_(0)
{
}

Targetable::Targetable(float health) :
    health_max_(health),
    health_current_(0)
{
}

Targetable::~Targetable()
{
}

