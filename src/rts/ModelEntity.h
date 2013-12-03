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

  static const uint32_t P_COLLIDABLE = 983556954;

  id_t getID() const {
    return id_;
  }
  virtual bool hasProperty(uint32_t property) const {
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
  bool isVisible() const;

  void setPosition(float t, const glm::vec2 &pos);
  void setPosition(float t, const glm::vec3 &pos);
  void setAngle(float t, float angle);
  void setVisible(bool visible);

  void setSize(const glm::vec2 &size);
  void setHeight(float height);

  // Graphics setters
  void setModelName(const std::string &meshName);
  void setModelName(std::string &&meshName);
  void setScale(const glm::vec3 &scale);
  void addExtraEffect(const RenderFunction &func);
  // Effect of color depends on material/model
  void setColor(const glm::vec3 &color);

  void render(float t);

private:
  id_t id_;
  Vec3Curve posCurve_;
	FloatCurve angleCurve_;

  glm::vec3 size_;

  bool visible_;

  std::string meshName_;
  glm::vec3 color_;
  glm::vec3 scale_;
  std::vector<RenderFunction> renderFuncs_;
};

}  // rts
