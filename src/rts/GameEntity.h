#ifndef SRC_RTS_ENTITY_H_
#define SRC_RTS_ENTITY_H_

#include <string>
#include <queue>
#include <glm/glm.hpp>
#include "common/Collision.h"
#include "common/Types.h"
#include "rts/Message.h"
#include "rts/ModelEntity.h"

class Checksum;

namespace rts {

// TODO(zack): merge with actor
class GameEntity : public ModelEntity {
 public:
  explicit GameEntity(id_t id,
      const std::string &name, const Json::Value &params);
  virtual ~GameEntity();

  static const uint32_t P_TARGETABLE = 463132888;
  static const uint32_t P_CAPPABLE = 815586235;
  static const uint32_t P_ACTOR = 913794634;
  static const uint32_t P_MOBILE = 1122719651;
  static const uint32_t P_UNIT = 118468328;

  virtual bool hasProperty(uint32_t property) const final override {
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

  const std::string& getName() const {
    return name_;
  }
  float getMaxSpeed() const {
    return maxSpeed_;
  }
  void setMaxSpeed(float max_speed) {
    maxSpeed_ = max_speed;
  }

  virtual void handleOrder(const Message &order) { }
  // Sets 'intention' like velocity, etc
  virtual void update(float dt);
  virtual void collide(const GameEntity *other, float dt) { }
  // Basically, integrate/apply the results of update/collide
  virtual void resolve(float dt) { }

  virtual void checksum(Checksum &chksum) const;

  std::queue<glm::vec3> getPathNodes() const;

  float distanceToEntity(const GameEntity *e) const;

  // helper functions for update
  // Don't move or rotate
  void remainStationary();
  // Rotates to face position
  void turnTowards(const glm::vec2 &pos);
  // Moves towards position as fast as possible (probably rotates)
  void moveTowards(const glm::vec2 &pos, float dt);
  // Teleports to the given position
  void warpPosition(const glm::vec2 &pos);

  const std::queue<glm::vec3>& getPathQueue() const {
    return pathQueue_;
  }

 protected:
  Json::Value getParam(const std::string &p) const;
  float fltParam(const std::string &p) const;
  std::string strParam(const std::string &p) const;
  glm::vec2 vec2Param(const std::string &p) const;
  bool hasParam(const std::string &p) const;

  std::queue<glm::vec3> pathQueue_;
  id_t playerID_;

 private:
  std::string name_;
  float maxSpeed_;

  std::set<uint32_t> properties_;
};
};  // namespace rts

#endif  // SRC_RTS_ENTITY_H_
