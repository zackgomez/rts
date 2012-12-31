#pragma once
#include <string>
#include "common/util.h"
#include "rts/Entity.h"

namespace rts {

class ModelEntity : public Entity
{
public:
  virtual ~ModelEntity() { }

  static const uint32_t P_RENDERABLE = 565038773;
  static const uint32_t P_COLLIDABLE = 983556954;

  virtual bool hasProperty(uint32_t property) const {
      return false;
  }

  virtual glm::vec2 getPosition(float dt) const = 0;
  virtual float getAngle(float dt) const = 0;
  virtual std::string getMeshName() const = 0;
  virtual glm::vec4 getColor() const = 0;
  virtual glm::mat4 getTransform(float dt) const = 0;

  virtual void render(float dt) = 0;
};

}  // rts
