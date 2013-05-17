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

class GameEntity : public ModelEntity {
 public:
  explicit GameEntity(id_t id,
      const std::string &name, const Json::Value &params,
      bool targetable = false, bool collidable = false);
  virtual ~GameEntity();

  static const uint32_t P_TARGETABLE = 463132888;
  static const uint32_t P_CAPPABLE = 815586235;
  static const uint32_t P_ACTOR = 913794634;

  virtual bool hasProperty(uint32_t property) const {
    if (property == P_TARGETABLE) {
      return targetable_;
    }
    else if (property == P_COLLIDABLE) {
      return collidable_;
    }
    else if (property == P_RENDERABLE) {
      return true;
    }
    return false;
  }

  // The player than owns this entity, or NO_PLAYER
  id_t getPlayerID() const {
    return playerID_;
  }
  id_t getTeamID() const;

  const std::string& getName() const {
    return name_;
  }

  virtual void handleMessage(const Message& msg);
  // Sets 'intention' like velocity, etc
  virtual void update(float dt) = 0;

  virtual void checksum(Checksum &chksum) const;

  std::queue<glm::vec3> getPathNodes() const;

  // helper functions for update
  // Don't move or rotate
  void remainStationary();
  // Rotates to face position
  void turnTowards(const glm::vec2 &pos, float dt);
  // Moves towards position as fast as possible (probably rotates)
  void moveTowards(const glm::vec2 &pos, float dt);

  const std::queue<glm::vec3>& getPathQueue() const {
    return pathQueue_;
  }

 protected:

  Json::Value getParam(const std::string &p) const;
  float fltParam(const std::string &p) const;
  std::string strParam(const std::string &p) const;
  glm::vec2 vec2Param(const std::string &p) const;
  bool hasParam(const std::string &p) const;

  // Used to change owners, e.g. for a capture
  void setPlayerID(id_t pid) {
    playerID_ = pid;
  }

  std::queue<glm::vec3> pathQueue_;

 private:
  id_t playerID_;
  std::string name_;

  bool targetable_;
  bool collidable_;
};
};  // namespace rts

#endif  // SRC_RTS_ENTITY_H_
