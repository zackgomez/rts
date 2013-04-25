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
const std::string ACTION = "ACTION";
};

class Actor : public GameEntity {
 public:
  struct Production {
    std::string name;
    float time;
    float max_time;
  };

  Actor(id_t id, const std::string &name, const Json::Value &params,
        bool targetable = true, bool collidable = true);
  virtual ~Actor();

  virtual bool hasProperty(uint32_t property) const;

  virtual void handleMessage(const Message &msg);
  virtual void update(float dt);

  float distanceToEntity(const GameEntity *e) const;

  Json::Value getActions() const;
  // TODO(zack)
  // protected virtual Json::Value getExtraActions() const;

  std::queue<Production> getProductionQueue() const {
    return production_queue_;
  }

  float getHealth() const {
    return health_;
  }
  float getMaxHealth() const {
    return fltParam("health");
  }

  // Returns sight radius
  float getSight() const {
    return fltParam("sight");
  }

  Clock::time_point getLastTookDamage() const {
    return lastTookDamage_;
  }
  
  void setTookDamage() {
    lastTookDamage_ = Clock::now();
  }

 protected:
  virtual void handleOrder(const Message &order);

  void handleAction(const Json::Value &action);
  void produce(const std::string &prod_name);

  void resetTexture();

  float health_;
  float melee_timer_;

  // Used by the render
  Clock::time_point lastTookDamage_;

  Weapon *rangedWeapon_, *meleeWeapon_;

  std::queue<Production> production_queue_;
};
};  // namespace rts

#endif  // SRC_RTS_ACTOR_H_
