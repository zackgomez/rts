#ifndef SRC_RTS_ENTITY_H_
#define SRC_RTS_ENTITY_H_

#include <string>
#include <glm/glm.hpp>
#include "common/Types.h"
#include "rts/Message.h"

class Checksum;

namespace rts {

class Entity {
 public:
  explicit Entity(const std::string &name, const Json::Value &params,
                  bool mobile = false, bool targetable = false);
  virtual ~Entity();

  bool isTargetable() const {
    return targetable_;
  }
  bool isMobile() const {
    return mobile_;
  }

  // This entity's unique id
  id_t getID() const {
    return id_;
  }
  // The player than owns this entity, or NO_PLAYER
  id_t getPlayerID() const {
    return playerID_;
  }
  id_t getTeamID() const;

  const std::string& getName() const {
    return name_;
  }
  // Basically returns the class of entity, must be overriden,
  // generally returns ClassName::TYPE
  virtual const std::string getType() const = 0;

  // This unit's position
  const glm::vec2 getPosition() const {
    return pos_;
  }
  // This unit's facing angle (relative to +x axis)
  const float getAngle() const {
    return angle_;
  }
  // This unit's bounding box
  const glm::vec2 getSize() const {
    return size_;
  }
  // Returns this entities height
  const float getHeight() const {
    return height_;
  }

  // Interpolation functions
  virtual glm::vec2 getPosition(float dt) const;
  virtual float getAngle(float dt) const;

  virtual void handleMessage(const Message& msg) = 0;
  virtual void update(float dt) = 0;

  float distanceBetweenEntities(const Entity *e) const;

  bool pointInEntity(const glm::vec2 &p);

 protected:
  static const glm::vec2 getDirection(float angle);
  const glm::vec2 getDirection() const;
  float angleToTarget(const glm::vec2 &pos) const;

  float param(const std::string &p) const;
  std::string strParam(const std::string &p) const;
  glm::vec2 vec2Param(const std::string &p) const;
  bool hasParam(const std::string &p) const;
  bool hasStrParam(const std::string &p) const;

  // Used to change owners, e.g. for a capture
  void setPlayerID(id_t pid) {
    playerID_ = pid;
  }

  glm::vec2 pos_;
  float angle_;
  glm::vec2 size_;
  float height_;

  float speed_;
  float turnSpeed_;

 private:
  static id_t lastID_;

  id_t id_;
  id_t playerID_;
  std::string name_;

  bool mobile_;
  bool targetable_;
};
};  // namespace rts

#endif  // SRC_RTS_ENTITY_H_
