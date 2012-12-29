#include "rts/CollisionObject.h"
#include <string>
#include "rts/MessageHub.h"

namespace rts {

LoggerPtr CollisionObject::logger_;
const std::string CollisionObject::TYPE = "COLLISION_OBJECT";

CollisionObject::CollisionObject(
    const std::string &name,
    const Json::Value &params)
  : GameEntity(name, params, false, false, true) {
  pos_ = toVec2(params["entity_pos"]);
  size_ = toVec2(params["entity_size"]);
  angle_ = params["entity_angle"].asFloat();
}

void CollisionObject::handleMessage(const Message &msg) {
  GameEntity::handleMessage(msg);
}

void CollisionObject::update(float dt) {
  GameEntity::update(dt);
}

bool CollisionObject::needsRemoval() const {
  return false;
}
};  // rts
