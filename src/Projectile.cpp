#include "Projectile.h"
#include "MessageHub.h"
#include "ParamReader.h"

namespace rts {

LoggerPtr Projectile::logger_;
const std::string Projectile::TYPE = "PROJECTILE";

Projectile::Projectile(const std::string &name, const Json::Value &params) :
  Entity(name, params, true),
  targetID_(NO_ENTITY)
{
  if (!logger_.get())
    logger_ = Logger::getLogger("Projectile");

  assert(params.isMember("projectile_target"));
  targetID_ = params["projectile_target"].asUInt64();
}

void Projectile::update(float dt)
{
  const Entity *targetEnt = MessageHub::get()->getEntity(targetID_);
  // If target doesn't exist for whatever reason, then I guess we're done
  if (!targetEnt)
  {
    MessageHub::get()->sendRemovalMessage(this);
    return;
  }
  // Always go directly towards target
  const glm::vec3 &targetPos = targetEnt->getPosition();
  angle_ = angleToTarget(targetPos);

  float dist = glm::length(targetPos - pos_);
  speed_ = param("speed");

  // If we would hit, then don't overshoot and send message (deal damage)
  if (dist < speed_ * dt)
  {
    speed_ = dist / dt;
    Message msg;
    msg["to"] = toJson(targetID_);
    msg["type"] = MessageTypes::ATTACK;
    msg["pid"] = toJson(getPlayerID());
    msg["damage"] = param("damage");
    MessageHub::get()->sendMessage(msg);
    MessageHub::get()->sendRemovalMessage(this);
  }

  // Move/rotate/etc
  Entity::update(dt);
}

void Projectile::handleMessage(const Message &msg)
{
  logger_->warning() << "Projectile ignoring message: " << msg << '\n';
}

}; // rts
