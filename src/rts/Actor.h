#ifndef SRC_RTS_ACTOR_H_
#define SRC_RTS_ACTOR_H_

#include <string>
#include <vector>
#include "common/Clock.h"
#include "rts/GameEntity.h"
#include "rts/UIAction.h"

namespace rts {

class Actor : public GameEntity {
 public:
  Actor(id_t id, const std::string &name, const Json::Value &params);
  virtual ~Actor();

  virtual void handleOrder(const Message &order) override;
  virtual void update(float dt) override;
  virtual void collide(const GameEntity *other, float dt) override final;
  virtual void resolve(float dt) override final;

  const std::vector<UIAction> &getActions() const;

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
    std::vector<glm::vec2> healths;
    glm::vec2 mana;
    glm::vec2 production;
    glm::vec2 capture;
  };
  UIInfo getUIInfo() const {
    return uiInfo_;
  }

 protected:
  void handleAction(const std::string &action_name, const Json::Value &order);

  void resetTexture();

  UIInfo uiInfo_;
  void updateUIInfo();

  std::vector<UIAction> actions_;
  void updateActions();

  // Used by the render
  Clock::time_point lastTookDamage_;
};
};  // namespace rts

#endif  // SRC_RTS_ACTOR_H_
