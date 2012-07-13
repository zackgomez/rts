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
    explicit Actor(const std::string &name);
    virtual ~Actor();
    virtual void update(float dt);

private:
    std::string actorName_;
    float attack_timer_;

protected:
    float attack_cooldown_;
    float attack_range_;
    float attack_arc_;
};

