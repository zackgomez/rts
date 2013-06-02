#include "rts/CollisionObject.h"
#include <string>
#include "rts/ResourceManager.h"

namespace rts {

CollisionObject::CollisionObject(
    id_t id,
    const std::string &name,
    const Json::Value &params)
  : GameEntity(id, name, params) {

  setScale(glm::vec3(2.f*getSize(), 1.f));
  setMeshName("square");

  GLuint texture = ResourceManager::get()->getTexture("collision-tex");
  setMaterial(createMaterial(glm::vec3(0.f), 0.f, texture));
}

void CollisionObject::handleMessage(const Message &msg) {
  GameEntity::handleMessage(msg);
}

void CollisionObject::update(float dt) {
  GameEntity::update(dt);
}
};  // rts
