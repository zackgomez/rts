#include "Unit.h"
#include "ParamReader.h"
#include "math.h"

LoggerPtr unitLogger;

Unit::Unit(int64_t playerID) :
    Entity(playerID),
    state_(new NullState(this))
{
    pos_ = glm::vec3(0, 0.5f, 0);
    radius_ = 0.5f;
    angle_ = 0.f;
    vel_ = glm::vec3(0.f);

    unitLogger = Logger::getLogger("Unit");

    maxSpeed_ = getParam("unit.maxSpeed");
}

void Unit::handleMessage(const Message &msg)
{
    if (msg["type"] == MessageTypes::ORDER &&
        msg["order_type"] == OrderTypes::MOVE)
    {
        unitLogger->info() << "Got a move order\n";
        state_->stop();
        delete state_;
        state_ = new MoveState(toVec3(msg["target"]), this);
    }

    else if (msg["type"] == MessageTypes::ORDER &&
            msg["order_type"] == OrderTypes::ATTACK)
    {
    	unitLogger->info() << "Got an attack order\n";
    	unitLogger->info() << "Imma go nuts on you id: " << msg["enemy_id"] << "\n";
    }
}

void Unit::update(float dt)
{
    state_->update(dt);

    UnitState *next;
    if ((next = state_->next()))
    {
        unitLogger->debug() << "changin state\n";
        delete state_;
        state_ = next;
    }

    pos_ += vel_ * dt;
}

bool Unit::needsRemoval() const
{
    return false;
}

void Unit::serialize(Json::Value &obj) const
{
    // TODO
}

void NullState::update(float dt)
{
    unit_->vel_ = glm::vec3(0.f);
}

MoveState::MoveState(const glm::vec3 &target, Unit *unit) :
    UnitState(unit),
    target_(target)
{
    // Make sure the unit stands on the terrain
    target_.y += unit_->getRadius(); 
    glm::vec3 diff = target_ - unit_->pos_;
    desired_angle_ = 180.0f * atan2(diff.z , diff.x) / M_PI;
}

void MoveState::update(float dt)
{
    // Turning
	if (unit_->angle_ != desired_angle_)
    {
        // Get delta in [-180, 180]
        float delAngle = desired_angle_ - unit_->angle_;
        while (delAngle > 180.f) delAngle -= 360.f;
        while (delAngle < -180.f) delAngle += 360.f;
        float turnRate = getParam("unit.turnRate");
        // Would overshoot, just set angle
        if (glm::sign(delAngle) != glm::sign(delAngle + turnRate * dt))
            unit_->angle_ = desired_angle_;
        else
        {
            unit_->angle_ += glm::sign(delAngle) * turnRate * dt;
            while (unit_->angle_ > 360.f) unit_->angle_ -= 360.f;
            while (unit_->angle_ < 0.f) unit_->angle_ += 360.f;

            unit_->vel_ = glm::vec3(0.f);
        }
    }
    // Moving
	else
    {
        float rad = M_PI / 180.f * unit_->angle_;
        unit_->vel_ = unit_->maxSpeed_ * glm::vec3(cosf(rad), 0, sinf(rad)); 
    }
}

UnitState * MoveState::next()
{
    glm::vec3 diff = target_ - unit_->pos_;
    float dist = glm::length(diff);

    if (dist < unit_->getRadius())
        return new NullState(unit_);
    return NULL;
}
