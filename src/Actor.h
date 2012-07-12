#pragma once
#include "Targetable.h"
#include <string>
#include <vector>

const static int ATTACK_TYPE_NORMAL = 0;

namespace OrderTypes
{
    const std::string MOVE = "MOVE";
    const std::string ATTACK = "ATTACK";
    const std::string STOP = "STOP";
};

class Actor :
    public Targetable
{
public:
    explicit Actor();
    explicit Actor(float health, float attack_damage, float attack_cooldown, int attack_type, float attack_range);
    virtual ~Actor() {}
    virtual void update(float dt);
protected:
    float attack_damage_;
    float attack_cooldown_;
    Targetable target_;
    int attack_type_;
    float attack_range_;
private:
    float attack_timer;
};
