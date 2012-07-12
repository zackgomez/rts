#include "Targetable.h"

Targetable::Targetable() :
    health(0)
{
}

Targetable::Targetable(float start_health) :
    health(start_health)
{
}

Targetable::~Targetable()
{
}
