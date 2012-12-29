#pragma once
#include "rts/RenderEntity.h"
#include "common/util.h"

namespace rts {

class GameEntityRenderShim :
  public RenderEntity {
public:
  virtual ~GameEntityRenderShim(void) { }

  virtual void render(float dt);

  // GameEntity required implements
  virtual glm::vec2 getPosition(float dt) const = 0;
  virtual float getAngle(float dt) const = 0;
  virtual const std::string getType() const = 0;
  virtual const std::string& getName() const = 0;
  virtual const glm::vec2 getSize() const = 0;

  // Eventual interface, now just adapters
  virtual glm::mat4 getTransform(float dt) const;
  virtual std::string getShaderName() const;
  virtual std::string getMeshName() const;
  virtual glm::vec4 getColor() const;
  virtual id_t getPlayerID() const = 0;

private:
  void renderMesh(const glm::mat4 &transform);
  void renderActor(const glm::vec3 &ndc);
};
}  // namespace rts
