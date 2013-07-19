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

  Clock::time_point getLastTookDamage(uint32_t part) const;
  void setTookDamage(int part_idx);

  // Used to change owners, e.g. for a capture
  void setPlayerID(id_t pid) {
    playerID_ = pid;
    resetTexture();
  }

  struct UIPartUpgrade {
    std::string name;
    // TODO resources and tooltip
  };
  struct UIPart {
    std::string name;
    glm::vec2 health;
    std::string tooltip;
    std::vector<UIPartUpgrade> upgrades;
  };

  struct UIInfo {
    std::vector<UIPart> parts;
    glm::vec2 mana;
    bool retreat;
    glm::vec2 capture;
    id_t capture_pid;
    char hotkey;
    std::string minimap_icon;
  };
  UIInfo getUIInfo() const {
    return uiInfo_;
  }

 protected:
  void handleAction(const std::string &action_name, const Json::Value &order);

  void resetTexture();

  UIInfo uiInfo_;
  void resetUIInfo();
  void updateUIInfo();

  std::vector<UIAction> actions_;
  void updateActions();

  // Used by the render
  std::map<uint32_t, Clock::time_point> lastTookDamage_;
};
};  // namespace rts

#endif  // SRC_RTS_ACTOR_H_
