#include "Projectile.h"
#include "MessageHub.h"
#include "ParamReader.h"

LoggerPtr Projectile::logger_;

Projectile::Projectile() :
    Mobile()
{
}

Projectile::Projectile(int64_t playerID, const glm::vec3 &pos,
        const std::string &type, eid_t targetID)
: Mobile(playerID, pos)
, targetID_(targetID)
, done_(false)
{
    if (!logger_.get())
        logger_ = Logger::getLogger("Projectile");

    name_ = type;
    radius_ = getParam(name_ + ".radius");
}

void Projectile::update(float dt)
{
    const Entity *targetEnt = MessageHub::get()->getEntity(targetID_);
    // If target doesn't exist for whatever reason, then I guess we're done
    if (!targetEnt)
    {
        done_ = true;
        return;
    }
    // Always go directly towards target
    const glm::vec3 &targetPos = targetEnt->getPosition();
    angle_ = angleToTarget(targetPos);

    float dist = glm::length(targetPos - pos_);
    speed_ = getParam(name_ + ".speed");

    // If we would hit, then don't overshoot and send message (deal damage)
    if (dist < speed_ * dt)
    {
        speed_ = dist / dt;
        Message msg;
        msg["to"] = toJson(targetID_);
        msg["type"] = MessageTypes::ATTACK;
        msg["pid"] = toJson(playerID_);
        msg["damage"] = getParam(name_ + ".damage");
        MessageHub::get()->sendMessage(msg);
        done_ = true;
    }

    // Move/rotate/etc
    Mobile::update(dt);
}

bool Projectile::needsRemoval() const
{
    return done_;
}

void Projectile::handleMessage(const Message &msg)
{
    logger_->warning() << "Projectile ignoring message: " << msg << '\n';
}

