#include "Targetable.h"

Targetable::Targetable() :
    health_max_(0)
{
}

Targetable::Targetable(float health) :
    health_max_(health)
{
}

Targetable::~Targetable()
{
}
