#pragma once
#include "Entity.h"

class Mobile :
    public Entity
{
public:
    explicit Mobile();
    explicit Mobile(int64_t playerID, glm::vec3 pos);
    virtual ~Mobile();
    virtual float getSpeed() const { return speed_; }
    virtual float getTurnSpeed() const { return turnSpeed_; }

    virtual void update(float dt);

protected:
    float speed_;
    float turnSpeed_;
};
