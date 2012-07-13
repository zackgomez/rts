#include "Actor.h"
#include "ParamReader.h"
#include "glm.h"

Actor::Actor(const std::string &name) :
    Targetable(getParam(name + ".health")),
    actorName_(name),
    attack_timer_(0.f),
    attack_cooldown_(getParam(name + ".cooldown")),
    attack_range_(getParam(name + ".range")),
    attack_arc_(getParam(name + ".firingArc"))
{
}

Actor::~Actor()
{
}

void Actor::update(float dt)
{
    attack_timer_ = glm::max(0.f, attack_timer_ - dt);
}

