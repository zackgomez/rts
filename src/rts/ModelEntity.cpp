#define GLM_SWIZZLE
#include "rts/ModelEntity.h"
#include "common/ParamReader.h"
#include "rts/Game.h"
#include "rts/Map.h"
#include "rts/ResourceManager.h"

namespace rts {

ModelEntity::ModelEntity(id_t id)
  : id_(id),
    posCurve_(glm::vec3(0.f)),
    angleCurve_(0.f),
    sizeCurve_(glm::vec3(0.f)),
    color_(0.f),
    scale_(1.f),
    visible_(true) {
}

ModelEntity::~ModelEntity() {
}


glm::vec2 ModelEntity::getSize2(float t) const {
  return glm::vec2(getSize3(t));
}
glm::vec3 ModelEntity::getSize3(float t) const {
  return sizeCurve_.linearSample(t);
}
float ModelEntity::getHeight(float t) const {
  return sizeCurve_.linearSample(t).z;
}
const Rect ModelEntity::getRect(float t) const {
  return Rect(glm::vec2(getPosition(t)), getSize2(t), glm::radians(getAngle(t)));
}

void ModelEntity::setPosition(float t, const glm::vec2 &pos) {
  // TODO(zack): make this only vec2 or vec3
  posCurve_.addKeyframe(t, glm::vec3(pos, 0.f));
}
void ModelEntity::setPosition(float t, const glm::vec3 &pos) {
  posCurve_.addKeyframe(t, pos);
}
void ModelEntity::setSize(float t, const glm::vec3 &size) {
  sizeCurve_.addKeyframe(t, size);
}
void ModelEntity::setAngle(float t, float angle) {
	angleCurve_.addKeyframe(t, angle);
}

bool ModelEntity::isVisible() const {
  return visible_;
}
void ModelEntity::setVisible(bool visible) {
  visible_ = visible;
}

void ModelEntity::setColor(const glm::vec3 &color) {
  color_ = color;
}
void ModelEntity::setModelName(const std::string &meshName) {
  meshName_ = meshName;
}

void ModelEntity::setModelName(std::string &&meshName) {
  meshName_ = std::move(meshName);
}

void ModelEntity::setScale(const glm::vec3 &scale) {
  scale_ = scale;
}

void ModelEntity::addExtraEffect(const RenderFunction &func) {
  renderFuncs_.push_back(func);
}

void ModelEntity::render(float t) {
  preRender(t);

  if (!isVisible()) {
    return;
  }
  if (meshName_.empty()) {
    return;
  }

  glm::mat4 transform = getTransform(t);

  // TODO(zack): make this part of a model object
  auto meshShader = ResourceManager::get()->getShader("unit");
  meshShader->makeActive();
  meshShader->uniform3f("baseColor", color_);
  // TODO(zack): add 'extra shader setup' hook here
  Model * mesh = ResourceManager::get()->getModel(meshName_);
  ::renderModel(transform, mesh);

  if (fltParam("local.debug.renderBoundingBox")) {
    auto shader = ResourceManager::get()->getShader("color");
    shader->makeActive();
    shader->uniform4f("color", glm::vec4(0.8f, 0.3f, 0.3f, 0.6f));

    auto pos = getPosition(t) + glm::vec3(0.f, 0.f, getHeight(t)/2.f);
    ::renderModel(
        glm::scale(
          glm::translate(glm::mat4(1.f), pos),
          0.5f * getSize3(t)),
        ResourceManager::get()->getModel("cube"));
  }

  // Now render additional effects
  for (auto it = renderFuncs_.begin(); it != renderFuncs_.end(); ) {
    if ((*it)(t)) {
      it++;
    } else {
      it = renderFuncs_.erase(it);
    }
  }
}

glm::mat4 ModelEntity::getTransform(float t) const {
  const float rotAngle = getAngle(t);

  return
    glm::scale(
      glm::rotate(
        // TODO(zack): remove this z position hack (used to make collision
        // objects (plane.obj) appear above the map
        glm::translate(glm::mat4(1.f), getPosition(t) + glm::vec3(.0f, .0f, .01f)),
        rotAngle, glm::vec3(0, 0, 1)),
      glm::vec3(scale_));
}

glm::vec2 ModelEntity::getPosition2(float t) const {
  return glm::vec2(getPosition(t));
}

glm::vec3 ModelEntity::getPosition(float t) const {
  return posCurve_.linearSample(t);
}

float ModelEntity::getAngle(float t) const {
  return angleCurve_.linearSample(t);
}

glm::vec2 ModelEntity::getDirection(float t) {
  float angle = getAngle(t);
  float rad = deg2rad(angle);
  return glm::vec2(cosf(rad), sinf(rad));
}
}  // rts
