#include "rts/Grenade.h"
#include "rts/MessageHub.h"

namespace rts {

Grenade::Grenade(id_t id, const std::string &name, const Json::Value &params)
  : GameEntity(id, name, params, false, false) {

  invariant(params.isMember("target_pos"), "missing target pos");
  targetPos_ = toVec3(params["target_pos"]);
  explodeTimer_ = fltParam("fuse");
}

void Grenade::update(float dt) {
  GameEntity::update(dt);

  auto diff = getPosition2() - glm::vec2(targetPos_);
  if (glm::dot(diff, diff) < 0.01f) {
    remainStationary();
  } else {
    moveTowards(glm::vec2(targetPos_), dt);
  }

  explodeTimer_ -= dt;
  if (explodeTimer_ <= 0.f) {
    // TODO(send damage message)
    MessageHub::get()->sendRemovalMessage(this);
  }
}

void Grenade::handleMessage(const Message &msg) {
  GameEntity::handleMessage(msg);
}

};  // rts
