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

    // override interpolation functions
    virtual glm::vec3 getPosition(float dt) const;
    virtual float getAngle(float dt) const;

protected:
    static const glm::vec3 getDirection(float angle);
    float angleToTarget(const glm::vec3 &pos) const;
    float speed_;
    float turnSpeed_;
};

