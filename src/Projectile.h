#include "Mobile.h"

class Projectile :
    public Mobile
{
public:
    Projectile();
    Projectile(int64_t playerID, const glm::vec3 &pos, 
            const std::string &type, eid_t targetID);
    virtual ~Projectile() {}

    virtual void update(float dt);
protected:
    eid_t targetID_;
    std::string projType_;
};

