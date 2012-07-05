#pragma once
#include "Entity.h"
#include <glm/glm.hpp>

class Unit :
    public Entity
{
public:
    Unit(int64_t playerID);
    virtual ~Unit() { }

    const glm::vec3 getPosition() const { return pos_; }
    const glm::vec2 getDirection() const { return dir_; }

    void render();
    void update(float dt);

private:
    glm::vec3 pos_;
    glm::vec2 dir_;
};

