#pragma once
#include <string>
#include "common/util.h"

namespace rts {

class RenderEntity
{
public:
  virtual ~RenderEntity() { }

  virtual id_t getID() const = 0;

  //virtual glm::vec2 getPosition(float dt) const = 0;
  virtual float getAngle() const = 0;
  virtual std::string getShaderName() const = 0;
  virtual std::string getMeshName() const = 0;
  virtual glm::vec4 getColor() const = 0;
  virtual glm::mat4 getTransform(float dt) const = 0;

  virtual void render(float dt) = 0;

  // TODO(zack): Instead of subclassing, make the idiom a script or
  // data as a k:v store for attributes.
};

}  // rts
