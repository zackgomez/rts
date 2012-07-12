#include "Projectile.h"

Projectile::Projectile() :
    Mobile()
{
}

Projectile::Projectile(int64_t playerID, const glm::vec3 &pos,
        const std::string &type, eid_t targetID)
: Mobile(playerID, pos)
, targetID_(targetID)
, projType_(type)
{
    // TODO verify that target is a targetable
}

void Projectile::update(float dt)
{
    // TODO update angle and speeds

    Mobile::update(dt);
}
