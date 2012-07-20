#pragma once
#include "Targetable.h"
#include "Message.h"
#include <string>
#include <vector>
#include <queue>

const static int ATTACK_TYPE_NORMAL = 0;

namespace OrderTypes
{
    const std::string MOVE = "MOVE";
    const std::string ATTACK = "ATTACK";
    const std::string ATTACK_MOVE = "AMOVE";
    const std::string STOP = "STOP";
};

class Actor :
    public Targetable
{
public:
    explicit Actor(const std::string &name);
    virtual ~Actor();
    virtual void update(float dt);
    virtual float getAttackTimer() const { return attack_timer_; }
    virtual float getAttackRange() const { return attack_range_; }
    struct Production
    {
        std::string name;
        float time;
        float max_time;
        Message msg;
    };        
    virtual std::queue<Production> getProductionQueue() const { return production_queue_; }

protected:
    void resetAttackTimer();

    float attack_range_;
    float attack_arc_;
    float attack_cooldown_;

    float attack_timer_;
    std::queue<Production> production_queue_;
};

