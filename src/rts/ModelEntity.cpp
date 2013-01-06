#include "rts/ModelEntity.h"
#include "rts/Game.h"
#include "rts/Map.h"
#include "rts/ResourceManager.h"

namespace rts {

ModelEntity::ModelEntity()
  : Entity(),
    material_(nullptr),
    scale_(1.f) {
}

ModelEntity::~ModelEntity() {
  freeMaterial(material_);
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

void ModelEntity::render(float dt) {
  if (meshName_.empty()) {
    return;
  }
  glm::mat4 transform = getTransform(dt);

  // TODO(zack): make this part of a model object
  GLuint meshProgram = ResourceManager::get()->getShader("unit");
  glUseProgram(meshProgram);

  // TODO(zack): make this part of a model object
  Mesh * mesh = ResourceManager::get()->getMesh(meshName_);
  ::renderMesh(transform, mesh, material_);
}

glm::mat4 ModelEntity::getTransform(float dt) const {
  const glm::vec2 pos2 = getPosition(dt);
  const float rotAngle = getAngle(dt);
  const glm::vec3 pos = glm::vec3(pos2, Game::get()->getMap()->getMapHeight(pos2));

  return
    glm::scale(
      glm::rotate(
        // TODO(zack): remove this z position hack (used to make collision
        // objects (plane.obj) appear above the map
        glm::translate(glm::mat4(1.f), pos + glm::vec3(.0f, .0f, .01f)),
        rotAngle, glm::vec3(0, 0, 1)),
      glm::vec3(scale_));
}
}  // rts
