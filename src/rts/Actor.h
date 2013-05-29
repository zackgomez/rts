#ifndef SRC_RTS_ACTOR_H_
#define SRC_RTS_ACTOR_H_

#include <queue>
#include <string>
#include <vector>
#include "common/Clock.h"
#include "rts/Effect.h"
#include "rts/GameEntity.h"
#include "rts/UIAction.h"

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
  Actor(id_t id, const std::string &name, const Json::Value &params,
        bool targetable = true, bool collidable = true);
  virtual ~Actor();

  virtual bool hasProperty(uint32_t property) const override;

  virtual void handleMessage(const Message &msg) override;
  virtual void update(float dt) override;
  virtual void collide(const GameEntity *other, float dt) override;

  float distanceToEntity(const GameEntity *e) const;

  const std::vector<UIAction> &getActions() const;

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
  // Used to change owners, e.g. for a capture
  void setPlayerID(id_t pid) {
    playerID_ = pid;
    resetTexture();
  }


  struct UIInfo {
    glm::vec2 health;
    glm::vec2 production;
    glm::vec2 capture;
  };

  const UIInfo& getUIInfo() const {
    return uiInfo_;
  }

 protected:
  virtual void handleOrder(const Message &order);

  void handleAction(const Json::Value &action);

  void resetTexture();

  float melee_timer_;

  void updateUIInfo();
  UIInfo uiInfo_;

  // Used by the render
  Clock::time_point lastTookDamage_;

  Weapon *rangedWeapon_, *meleeWeapon_;

  std::map<std::string, Effect> effects_;
  std::vector<UIAction> actions_;
};
};  // namespace rts

#endif  // SRC_RTS_ACTOR_H_
