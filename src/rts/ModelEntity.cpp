#define GLM_SWIZZLE_XYZW
#include "rts/ModelEntity.h"
#include "common/ParamReader.h"
#include "rts/Game.h"
#include "rts/Map.h"
#include "rts/ResourceManager.h"

namespace rts {

ModelEntity::ModelEntity(id_t id)
  : Entity(id),
    pos_(HUGE_VAL),
    angle_(0.f),
    size_(0.f),
    speed_(0.f),
    bumpVel_(0.f),
    material_(nullptr),
    scale_(1.f),
    visible_(true) {
}

ModelEntity::~ModelEntity() {
  freeMaterial(material_);
}

Rect ModelEntity::getRect() const {
  return Rect(glm::vec2(pos_), getSize(), glm::radians(angle_));
}
const Rect ModelEntity::getRect(float dt) const {
  return Rect(glm::vec2(getPosition(dt)), getSize(), glm::radians(getAngle(dt)));
}

const glm::vec3 ModelEntity::getVelocity() const {
  return glm::vec3(getDirection() * speed_, 0.f) + bumpVel_;
}

void ModelEntity::setPosition(const glm::vec2 &pos) {
  pos_ = glm::vec3(pos, pos_.z);
}
void ModelEntity::setPosition(const glm::vec3 &pos) {
  pos_ = pos;
}
void ModelEntity::setSize(const glm::vec2 &size) {
  size_.xy = size;
}
void ModelEntity::setAngle(float angle) {
  angle_ = angle;
}
void ModelEntity::setHeight(float height) {
  size_.z = height;
}
void ModelEntity::setSpeed(float speed) {
  speed_ = speed;
}
void ModelEntity::setBumpVel(const glm::vec3 &bumpVel) {
  bumpVel_ = bumpVel;
}
void ModelEntity::addBumpVel(const glm::vec3 &delta) {
  bumpVel_ += delta;
}

void ModelEntity::setVisible(bool visible) {
  visible_ = visible;
}

void ModelEntity::setMaterial(Material *material) {
  freeMaterial(material_);
  material_ = material;
}

void ModelEntity::setMeshName(const std::string &meshName) {
  meshName_ = meshName;
}

void ModelEntity::setMeshName(std::string &&meshName) {
  meshName_ = std::move(meshName);
}

void ModelEntity::setScale(const glm::vec3 &scale) {
  scale_ = scale;
}

void ModelEntity::addExtraEffect(const RenderFunction &func) {
  renderFuncs_.push_back(func);
}

void ModelEntity::render(float dt) {
  if (!visible_) {
    return;
  }
  if (meshName_.empty()) {
    return;
  }
  glm::mat4 transform = getTransform(dt);

  // TODO(zack): make this part of a model object
  auto meshShader = ResourceManager::get()->getShader("unit");
  meshShader->makeActive();
  // TODO(zack): make this part of a model object
  Mesh * mesh = ResourceManager::get()->getMesh(meshName_);
  ::renderMeshMaterial(transform, mesh, material_);

  if (fltParam("local.debug.renderBoundingBox")) {
    auto shader = ResourceManager::get()->getShader("color");
    shader->makeActive();
    shader->uniform4f("color", glm::vec4(0.8f, 0.3f, 0.3f, 0.6f));

    auto pos = getPosition(dt) + glm::vec3(0.f, 0.f, getHeight()/2.f);
    ::renderMesh(
        glm::scale(
          glm::translate(glm::mat4(1.f), pos),
          0.5f * glm::vec3(getSize(), getHeight())),
        ResourceManager::get()->getMesh("cube"));
  }

  // Now render additional effects
  for (auto&& renderer : renderFuncs_) {
    renderer(dt);
  }
}

void ModelEntity::integrate(float dt) {
  pos_ = getPosition(dt);
}

glm::mat4 ModelEntity::getTransform(float dt) const {
  const float rotAngle = getAngle(dt);

  return
    glm::scale(
      glm::rotate(
        // TODO(zack): remove this z position hack (used to make collision
        // objects (plane.obj) appear above the map
        glm::translate(glm::mat4(1.f), getPosition(dt) + glm::vec3(.0f, .0f, .01f)),
        rotAngle, glm::vec3(0, 0, 1)),
      glm::vec3(scale_));
}

glm::vec2 ModelEntity::getPosition2(float dt) const {
  return glm::vec2(getPosition(dt));
}

glm::vec3 ModelEntity::getPosition(float dt) const {
  return pos_ + getVelocity() * dt;
}

float ModelEntity::getAngle(float dt) const {
  return angle_;
}

glm::vec2 ModelEntity::getDirection(float angle) {
  float rad = deg2rad(angle);
  return glm::vec2(cosf(rad), sinf(rad));
}

glm::vec2 ModelEntity::getDirection() const {
  return getDirection(angle_);
}

float ModelEntity::angleToTarget(const glm::vec2 &target) const {
  glm::vec2 delta = target - getPosition2();
  return rad2deg(atan2(delta.y , delta.x));
}

bool ModelEntity::pointInEntity(const glm::vec2 &p) {
  return pointInBox(p, glm::vec2(pos_), size_.xy, angle_);
}
}  // rts
