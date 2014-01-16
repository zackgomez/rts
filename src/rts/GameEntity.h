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

typedef std::set<id_t> VisibilitySet;

class GameEntity : public ModelEntity {
 public:
  static GameEntity* cast(ModelEntity *e);
  static const GameEntity* cast(const ModelEntity *e);
  explicit GameEntity(id_t id);
  virtual ~GameEntity();

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
    std::vector<glm::vec3> path;

    UIInfo();
  };

  virtual bool hasProperty(uint32_t property) const final override;

  const std::string& getGameID() const {
    return gameID_;
  }
  Clock::time_point getLastTookDamage(uint32_t part) const;
  const std::vector<UIAction> &getActions() const {
    return actions_;
  }
  UIInfo getUIInfo(float t) const;

  // The player that owns this entity, or NO_PLAYER
  id_t getPlayerID(float t) const;
  id_t getTeamID(float t) const;
  bool getAlive(float t) const;

  float getSight(float t) const {
    return sight_;
  }
  void setSight(float t, float sight) {
    sight_ = sight;
  }

  void setGameID(const std::string &id) {
    gameID_ = id;
  }
  void setAlive(float t, bool alive);
  void setPlayerID(float t, id_t pid);
  void setTeamID(float t, id_t tid);
  void setUIInfo(float t, const UIInfo &ui_info);
  void setActions(float t, const std::vector<UIAction> &actions) {
    actions_ = actions;
  }
  void setTookDamage(int part_idx);

  void addProperty(uint32_t property) {
    properties_.insert(property);
  }
  void clearProperties() {
    properties_.clear();
  }

  bool isVisibleTo(float t, id_t pid) const;
  void setVisibilitySet(float t, const VisibilitySet &map);

 protected:
  virtual void preRender(float t) final override;

 private:
  std::string gameID_;
  Curve<id_t> playerCurve_;
  Curve<id_t> teamCurve_;
  Curve<bool> aliveCurve_;
  Curve<VisibilitySet> visibilityCurve_;
  Curve<UIInfo> uiInfoCurve_;

  // TODO(zack): kill this
  std::map<uint32_t, Clock::time_point> lastTookDamage_;

  std::set<uint32_t> properties_;
  float sight_;
  UIInfo uiInfo_;
  std::vector<UIAction> actions_;
};
};  // namespace rts

#endif  // SRC_RTS_ENTITY_H_
