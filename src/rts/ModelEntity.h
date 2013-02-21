#pragma once
#include <functional>
#include <string>
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

  void setMaterial(Material *material);
  void setMeshName(const std::string &meshName);
  void setMeshName(std::string &&meshName);
  void setScale(const glm::vec3 &scale);
  typedef std::function<void(float)> RenderFunction;
  void addExtraEffect(const RenderFunction &func);

  virtual glm::vec2 getPosition(float dt) const = 0;
  virtual float getAngle(float dt) const = 0;
  virtual glm::mat4 getTransform(float dt) const;

  virtual void render(float dt) final;

private:
  std::string meshName_;
  Material *material_;
  glm::vec3 scale_;
  std::vector<RenderFunction> renderFuncs_;
};

}  // rts
