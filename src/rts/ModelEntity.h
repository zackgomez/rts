#pragma once
#include <functional>
#include <string>
#include "common/Collision.h"
#include "common/util.h"
#include "rts/EffectFactory.h"
#include "rts/Entity.h"
#include "rts/Graphics.h"

namespace rts {

class ModelEntity : public Entity
{
public:
  ModelEntity(id_t id);
  virtual ~ModelEntity();

  static const uint32_t P_RENDERABLE = 565038773;
  static const uint32_t P_COLLIDABLE = 983556954;

  virtual bool hasProperty(uint32_t property) const {
    // TODO(zack): remove, this is a hack for collision objects
    if (property == P_RENDERABLE) {
      return true;
    }
    return false;
  }

  glm::vec2 getPosition2() const {
    return glm::vec2(pos_);
  }
  const glm::vec3& getPosition() const {
    return pos_;
  }
  // This unit's facing angle (relative to +x axis)
  float getAngle() const {
    return angle_;
  }
  glm::vec2 getDirection() const;
  // This unit's bounding box
  glm::vec2 getSize() const {
    return glm::vec2(size_);
  }
  // Returns this entities height
  float getHeight() const {
    return size_.z;
  }
  // Bounding rectangle
  Rect getRect() const;
  const glm::vec3 getVelocity() const;
  float getSpeed() const {
    return speed_;
  }
  const glm::vec3& getBumpVel() const {
    return bumpVel_;
  }

  // Interpolation functions
  glm::vec2 getPosition2(float dt) const;
  glm::vec3 getPosition(float dt) const;
  float getAngle(float dt) const;
  glm::mat4 getTransform(float dt) const;
  const Rect getRect(float dt) const;

  void setPosition(const glm::vec2 &pos);
  void setPosition(const glm::vec3 &pos);
  void setSize(const glm::vec2 &size);
  void setAngle(float angle);
  void setHeight(float height);
  void setTurnSpeed(float turn_speed);
  void setSpeed(float speed);
  void setBumpVel(const glm::vec3 &vel);
  void addBumpVel(const glm::vec3 &delta);


  // Graphics setters
  void setVisible(bool visible);
  void setMaterial(Material *material);
  void setModelName(const std::string &meshName);
  void setModelName(std::string &&meshName);
  void setScale(const glm::vec3 &scale);
  void addExtraEffect(const RenderFunction &func);

  // graphics getters
  bool isVisible() const {
    return visible_;
  }

  void render(float dt);
  // Integrates position using velocities and timestep
  void integrate(float dt);

  // Some queries
  bool pointInEntity(const glm::vec2 &p);
  float angleToTarget(const glm::vec2 &pos) const;

protected:
  static glm::vec2 getDirection(float angle);

private:
  glm::vec3 pos_;
  float angle_;
  glm::vec3 size_;

  // velocity not related to forward movement
  glm::vec3 bumpVel_;

  float speed_;

  bool visible_;

  std::string meshName_;
  Material *material_;
  glm::vec3 scale_;
  std::vector<RenderFunction> renderFuncs_;
};

}  // rts
