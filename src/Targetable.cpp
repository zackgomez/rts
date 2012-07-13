#include "Targetable.h"

Targetable::Targetable() :
    health_max_(0.f),
    health_(0.f)
{
}

Targetable::Targetable(float health) :
    health_max_(health),
    health_(health)
{
}

Targetable::~Targetable()
{
}

