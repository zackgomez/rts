#include "Mobile.h"

Mobile::Mobile() :
    Entity(NO_PLAYER, glm::vec3(0.f))
{
}

Mobile::Mobile(int64_t id, glm::vec3 pos) :
    Entity(id, pos),
    speed_(0.f),
    turnSpeed_(0.f)
{
}

Mobile::~Mobile() {}

void Mobile::update(float dt)
{
    // rotate
    angle_ += turnSpeed_ * dt;
    // clamp to [0, 360]
    while (angle_ > 360.f) angle_ -= 360.f;
    while (angle_ < 0.f) angle_ += 360.f;

    // move
    float rad = deg2rad(angle_);
    glm::vec3 vel = speed_ * glm::vec3(cosf(rad), 0, sinf(rad)); 
    pos_ += vel * dt;
}

