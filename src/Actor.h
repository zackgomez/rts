#pragma once
#include <queue>
#include <string>
#include <vector>
#include "Entity.h"
#include "Logger.h"

namespace OrderTypes
{
    const std::string MOVE = "MOVE";
    const std::string ATTACK = "ATTACK";
    const std::string ATTACK_MOVE = "AMOVE";
    const std::string STOP = "STOP";
    const std::string ENQUEUE = "ENQUEUE";
};

class Actor : public Entity
{
public:
    struct Production
    {
        std::string name;
        float time;
        float max_time;
    };        

    Actor(const std::string &name, const Json::Value &params,
            bool mobile = false, bool targetable = true);
    virtual ~Actor();

    virtual void handleMessage(const Message &msg);
    virtual void update(float dt);
    virtual std::queue<Production> getProductionQueue() const { return production_queue_; }

    float getAttackTimer() const { return attack_timer_; }
    float getAttackRange() const { return attack_range_; }
    float getHealth() const { return health_; }
    float getMaxHealth() const { return health_max_; }

protected:
    virtual void handleOrder(const Message &order);

    void resetAttackTimer();
    void enqueue(const Message &queue_order);
    void produce(const std::string &prod_name);

    float health_max_;
    float health_;

    float attack_range_;
    float attack_arc_;
    float attack_cooldown_;

    float attack_timer_;
    std::queue<Production> production_queue_;

private:
    static LoggerPtr logger_;
};

