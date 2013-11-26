#pragma once
#include <functional>
#include <string>
#include "common/Collision.h"
#include "common/util.h"
#include "rts/Curves.h"
#include "rts/EffectFactory.h"
#include "rts/Graphics.h"

namespace rts {

class ModelEntity {
public:
  ModelEntity(id_t id);
  virtual ~ModelEntity();

  static const uint32_t P_RENDERABLE = 565038773;
  static const uint32_t P_COLLIDABLE = 983556954;

  id_t getID() const {
    return id_;
  }
  virtual bool hasProperty(uint32_t property) const {
    // TODO(zack): remove, this is a hack for collision objects
    if (property == P_RENDERABLE) {
      return true;
    }
    return false;
  }

  // This unit's bounding box
  glm::vec2 getSize() const {
    return glm::vec2(size_);
  }
  // Returns this entities height
  float getHeight() const {
    return size_.z;
  }

  // Interpolation functions
  glm::vec2 getPosition2(float t) const;
  glm::vec3 getPosition(float t) const;
  glm::vec2 getDirection(float t);
  float getAngle(float t) const;
  glm::mat4 getTransform(float t) const;
  const Rect getRect(float t) const;

  void setPosition(float t, const glm::vec2 &pos);
  void setPosition(float t, const glm::vec3 &pos);
  void setSize(const glm::vec2 &size);
  void setAngle(float angle);
  void setHeight(float height);

  // Graphics setters
  void setVisible(bool visible);
  void setModelName(const std::string &meshName);
  void setModelName(std::string &&meshName);
  void setScale(const glm::vec3 &scale);
  void addExtraEffect(const RenderFunction &func);
  // Effect of color depends on material/model
  void setColor(const glm::vec3 &color);

  // graphics getters
  bool isVisible() const {
    return visible_;
  }

  void render(float t);

private:
  id_t id_;
  Vec3Curve posCurve_;
  float angle_;
  glm::vec3 size_;

  bool visible_;

  std::string meshName_;
  glm::vec3 color_;
  glm::vec3 scale_;
  std::vector<RenderFunction> renderFuncs_;
};

}  // rts
