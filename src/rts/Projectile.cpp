#include "rts/Projectile.h"
#include "common/ParamReader.h"
#include "rts/MessageHub.h"

namespace rts {

Projectile::Projectile(id_t id, const std::string &name, const Json::Value &params)
  : Actor(id, name, params),
    targetID_(NO_ENTITY) {

  invariant(params.isMember("target_id"), "missing target");
  targetID_ = toID(params["target_id"]);

  setPosition(glm::vec3(getPosition2(), 0.5f));
  setMeshName(strParam("model"));
  setMaterial(createMaterial(
        glm::vec3(0.6f),
        10.f));
}

void Projectile::update(float dt) {
  Actor::update(dt);

  const GameEntity *targetEnt = Game::get()->getEntity(targetID_);
  // If target doesn't exist for whatever reason, then I guess we're done
  if (!targetEnt) {
    Game::get()->destroyEntity(getID());
    return;
  }
  // Always go directly towards target
  const glm::vec2 &targetPos = targetEnt->getPosition2();
  setAngle(angleToTarget(targetPos));

  float dist = glm::length(targetPos - getPosition2());
  setSpeed(fltParam("speed"));

  // If we would hit, then don't overshoot and send message (deal damage)
  if (dist < getSpeed() * dt) {
    setSpeed(dist / dt);
    Message msg;
    msg["to"] = toJson(targetID_);
    msg["from"] = toJson(getPlayerID());
    msg["type"] = MessageTypes::ATTACK;
    msg["pid"] = toJson(getPlayerID());
    msg["damage"] = fltParam("damage");
    msg["damage_type"] = "ranged";
    MessageHub::get()->sendMessage(msg);
    Game::get()->destroyEntity(getID());
  }
}
};  // rts
