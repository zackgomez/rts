#ifndef SRC_RTS_ACTOR_H_
#define SRC_RTS_ACTOR_H_

#include <queue>
#include <string>
#include <vector>
#include "common/Clock.h"
#include "rts/GameEntity.h"
#include "rts/UIAction.h"

namespace rts {

class Weapon;

namespace OrderTypes {
const std::string MOVE = "MOVE";
const std::string ATTACK = "ATTACK";
const std::string CAPTURE = "CAPTURE";
const std::string STOP = "STOP";
const std::string ACTION = "ACTION";
};

class Actor : public GameEntity {
 public:
  Actor(id_t id, const std::string &name, const Json::Value &params);
  virtual ~Actor();

  virtual void handleMessage(const Message &msg) override;
  virtual void update(float dt) override;
  virtual void collide(const GameEntity *other, float dt) override final;

  const std::vector<UIAction> &getActions() const;

  // Returns sight radius
  float getSight() const;

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
  void handleAction(const std::string &action_name, const Json::Value &order);

  void resetTexture();

  float melee_timer_;

  UIInfo uiInfo_;
  void updateUIInfo();

  std::vector<UIAction> actions_;
  void updateActions();

  // Used by the render
  Clock::time_point lastTookDamage_;

  Weapon *rangedWeapon_, *meleeWeapon_;
};
};  // namespace rts

#endif  // SRC_RTS_ACTOR_H_
