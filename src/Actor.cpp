#include "Actor.h"

Actor::Actor() :
    Targetable()
{
}

Actor::Actor(float health, float attack_damage, float attack_cooldown, int attack_type, float attack_range) :
    Targetable(health),
    attack_damage_(attack_damage),
    attack_cooldown_(attack_cooldown),
    attack_type_(attack_type),
    attack_range_(attack_range)
{
}

void Actor::update(float dt)
{
}
