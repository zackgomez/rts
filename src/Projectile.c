#include "Projectile.h"

Projectile::Projectile() :
    Mobile()
{
}

Projectile::Projectile(glm::vec2 pos, float speed, Targetable *target)
    Mobile(NO_PLAYER, speed, 0),
    target_(target)
{
}
