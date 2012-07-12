#include "Mobile.h"
#include "Targetable.h"
#include "glm.h"

class Projectile :
    public Mobile
{
public:
    explicit Projectile();
    explicit Projectile(glm::vec2 pos, float speed, Targetable *target);
    virtual ~Projectile() {}
    virtual void update(float dt);
protected:
    Targetable *target_;
};
