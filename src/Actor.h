#pragma once
#include <queue>
#include <string>
#include <vector>
#include "Entity.h"
#include "Logger.h"

namespace rts {

class Weapon;

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

  float getHealth() const { return health_; }
  float getMaxHealth() const { return param("health"); }

protected:
  virtual void handleOrder(const Message &order);

  void enqueue(const Message &queue_order);
  void produce(const std::string &prod_name);

  float health_;
  float melee_timer_;

  Weapon *rangedWeapon_, *meleeWeapon_;

  std::queue<Production> production_queue_;

private:
  static LoggerPtr logger_;
};

}; // namespace rts
