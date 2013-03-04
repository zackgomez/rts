#ifndef SRC_RTS_ACTOR_H_
#define SRC_RTS_ACTOR_H_

#include <queue>
#include <string>
#include <vector>
#include "common/Clock.h"
#include "rts/GameEntity.h"
#include "common/Logger.h"

namespace rts {

class Weapon;

namespace OrderTypes {
const std::string MOVE = "MOVE";
const std::string ATTACK = "ATTACK";
const std::string CAPTURE = "CAPTURE";
const std::string ATTACK_MOVE = "AMOVE";
const std::string STOP = "STOP";
const std::string ENQUEUE = "ENQUEUE";
};

class Actor : public GameEntity {
 public:
  struct Production {
    std::string name;
    float time;
    float max_time;
  };

  Actor(const std::string &name, const Json::Value &params,
        bool targetable = true, bool collidable = true);
  virtual ~Actor();

  virtual bool hasProperty(uint32_t property) const {
    if (property == P_ACTOR) {
      return true;
    }
    return GameEntity::hasProperty(property);
  }

  virtual void handleMessage(const Message &msg);
  virtual void update(float dt);
  virtual std::queue<Production> getProductionQueue() const {
    return production_queue_;
  }

  float getHealth() const {
    return health_;
  }
  float getMaxHealth() const {
    return param("health");
  }

  // Returns sight radius
  float getSight() const {
    return param("sight");
  }

  Clock::time_point getLastTookDamage() const {
    return lastTookDamage_;
  }
  
  void setTookDamage() {
    lastTookDamage_ = Clock::now();
  }

 protected:
  virtual void handleOrder(const Message &order);

  void enqueue(const Message &queue_order);
  void produce(const std::string &prod_name);

  float health_;
  float melee_timer_;

  // Used by the render
  Clock::time_point lastTookDamage_;

  Weapon *rangedWeapon_, *meleeWeapon_;

  std::queue<Production> production_queue_;
};
};  // namespace rts

#endif  // SRC_RTS_ACTOR_H_
