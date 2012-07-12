#include "Mobile.h"

Mobile::Mobile() :
    Entity(NO_PLAYER, glm::vec3(0.f))
{
}

Mobile::Mobile(int64_t id, glm::vec3 pos, float speed, float turnSpeed) :
    Entity(id, pos),
    speed_(speed),
    turnSpeed_(turnSpeed)
{
}

Mobile::~Mobile() {}
