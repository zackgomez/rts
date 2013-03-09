#pragma once
#include <functional>
#include <string>
#include "common/Collision.h"
#include "common/util.h"
#include "rts/Entity.h"
#include "rts/Graphics.h"

namespace rts {

class ModelEntity : public Entity
{
public:
  ModelEntity();
  virtual ~ModelEntity();

  static const uint32_t P_RENDERABLE = 565038773;
  static const uint32_t P_COLLIDABLE = 983556954;

  virtual bool hasProperty(uint32_t property) const {
      return false;
  }

  // This unit's position
  const glm::vec2& getPosition() const {
    return pos_;
  }
  // This unit's facing angle (relative to +x axis)
  float getAngle() const {
    return angle_;
  }
  const glm::vec2 getDirection() const;
  // This unit's bounding box
  const glm::vec2& getSize() const {
    return size_;
  }
  // Returns this entities height
  float getHeight() const {
    return height_;
  }
  // Bounding rectangle
  Rect getRect() const {
    return Rect(pos_, size_, glm::radians(angle_));
  }
  const glm::vec2 getVelocity() const {
    return getDirection() * speed_;
  }
  float getSpeed() const {
    return speed_;
  }
  float getTurnSpeed() const {
    return turnSpeed_;
  }

  // Interpolation functions
  glm::vec2 getPosition(float dt) const;
  float getAngle(float dt) const;
  glm::mat4 getTransform(float dt) const;
  const Rect getRect(float dt) const;

  void setPosition(const glm::vec2 &pos);
  void setSize(const glm::vec2 &size);
  void setAngle(float angle);
  void setHeight(float height);
  void setTurnSpeed(float turn_speed);
  void setSpeed(float speed);

  void setMaterial(Material *material);
  void setMeshName(const std::string &meshName);
  void setMeshName(std::string &&meshName);
  void setScale(const glm::vec3 &scale);
  typedef std::function<void(float)> RenderFunction;
  void addExtraEffect(const RenderFunction &func);

  void render(float dt);
  // Integrates position using velocities and timestep
  void integrate(float dt);

  // Some queries
  bool pointInEntity(const glm::vec2 &p);
  float angleToTarget(const glm::vec2 &pos) const;

protected:
  static const glm::vec2 getDirection(float angle);

private:
  glm::vec2 pos_;
  float angle_;
  glm::vec2 size_;
  float height_;

  float speed_;
  float turnSpeed_;

  std::string meshName_;
  Material *material_;
  glm::vec3 scale_;
  std::vector<RenderFunction> renderFuncs_;
};

}  // rts
