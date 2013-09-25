#ifndef SRC_RTS_ENTITY_H_
#define SRC_RTS_ENTITY_H_

#include <string>
#include <queue>
#include <glm/glm.hpp>
#include "common/Clock.h"
#include "common/Collision.h"
#include "common/Types.h"
#include "rts/ModelEntity.h"
#include "rts/UIAction.h"

class Checksum;

namespace rts {

// TODO(zack): merge with actor
class GameEntity : public ModelEntity {
 public:
  explicit GameEntity(id_t id,
      const std::string &name, const Json::Value &params);
  virtual ~GameEntity();

  static const uint32_t P_GAMEENTITY = 293013864;
  static const uint32_t P_TARGETABLE = 463132888;
  static const uint32_t P_CAPPABLE = 815586235;
  static const uint32_t P_ACTOR = 913794634;
  static const uint32_t P_MOBILE = 1122719651;
  static const uint32_t P_UNIT = 118468328;

  struct UIPartUpgrade {
    std::string part;
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
    Json::Value extra;
  };

  virtual bool hasProperty(uint32_t property) const final override {
    if (property == P_GAMEENTITY) {
      return true;
    }
    return properties_.count(property);
  }
  void setProperty(uint32_t property, bool val) {
    if (val) {
      properties_.insert(property);
    } else {
      properties_.erase(property);
    }
  }

  // The player that owns this entity, or NO_PLAYER
  id_t getPlayerID() const {
    return playerID_;
  }
  id_t getTeamID() const;

  float getMaxSpeed() const {
    return maxSpeed_;
  }
  void setMaxSpeed(float max_speed) {
    maxSpeed_ = max_speed;
  }
  float getSight() const {
    return sight_;
  }
  void setSight(float sight) {
    sight_ = sight;
  }
  void setPlayerID(id_t pid);
  void setGameID(const std::string &id) {
    gameID_ = id;
  }
  const std::string& getGameID() const {
    return gameID_;
  }

  void setUIInfo(const UIInfo &ui_info) {
    uiInfo_ = ui_info;
  };
  void setActions(const std::vector<UIAction> &actions) {
    actions_ = actions;
  }
  void setTookDamage(int part_idx);
  Clock::time_point getLastTookDamage(uint32_t part) const;
  const std::vector<UIAction> &getActions() const {
    return actions_;
  }
  UIInfo getUIInfo() const {
    return uiInfo_;
  }

  void collide(const GameEntity *other, float dt);
  // TODO(zack): remove this function
  void resolve(float dt);

  void checksum(Checksum &chksum) const;

  std::vector<glm::vec3> getPathNodes() const;

  float distanceToEntity(const GameEntity *e) const;

  // helper functions for update
  // Don't move or rotate
  void remainStationary();
  // Rotates to face position
  void turnTowards(const glm::vec2 &pos);
  // Moves towards position as fast as possible (probably rotates)
  void moveTowards(const glm::vec2 &pos);
  // Teleports to the given position
  void warpPosition(const glm::vec2 &pos);

  const std::vector<glm::vec3>& getPathQueue() const {
    return pathQueue_;
  }

 protected:
  id_t playerID_;
  std::string gameID_;

  bool warp_;
  glm::vec2 warpTarget_;

  glm::vec2 lastTargetPos_;
  std::vector<glm::vec3> pathQueue_;

 private:
  std::string name_;
  float maxSpeed_;
  float sight_;

  std::set<uint32_t> properties_;

  // TODO(zack): kill this
  std::map<uint32_t, Clock::time_point> lastTookDamage_;
  UIInfo uiInfo_;
  std::vector<UIAction> actions_;

  void resetTexture();
  void updateUIInfo();
  void updateActions();
};
};  // namespace rts

#endif  // SRC_RTS_ENTITY_H_
