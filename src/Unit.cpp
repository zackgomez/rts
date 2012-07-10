#include "Unit.h"
#include "ParamReader.h"
#include "math.h"

LoggerPtr Unit::logger_;

Unit::Unit(int64_t playerID, const glm::vec3 &pos) :
    Entity(playerID),
    state_(new NullState(this))
{
    pos_ = pos;
    radius_ = 0.5f;
    angle_ = 0.f;
    vel_ = glm::vec3(0.f);

    if (!logger_.get())
        logger_ = Logger::getLogger("Unit");
}

void Unit::handleMessage(const Message &msg)
{
    if (msg["type"] == MessageTypes::ORDER &&
        msg["order_type"] == OrderTypes::MOVE)
    {
        logger_->info() << "Got a move order\n";
        state_->stop();
        delete state_;
        state_ = new MoveState(toVec3(msg["target"]), this);
    }
    else if (msg["type"] == MessageTypes::ORDER &&
            msg["order_type"] == OrderTypes::ATTACK)
    {
    	logger_->info() << "Got an attack order\n";
    	logger_->info() << "Imma go nuts on you id: " << msg["enemy_id"] << "\n";
    }
    else if (msg["type"] == MessageTypes::ORDER &&
            msg["order_type"] == OrderTypes::STOP)
    {
        logger_->info() << "Got a move order\n";
        state_->stop();
        delete state_;
        state_ = new NullState(this);
    }
    else
    {
        logger_->warning() << "Unit got unknown message: "
            << msg.toStyledString() << '\n';
    }
}

void Unit::update(float dt)
{
    state_->update(dt);

    UnitState *next;
    if ((next = state_->next()))
    {
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
    obj["state"] = state_->getName();
    obj["vel"] = toJson(vel_);
    state_->serialize(obj);
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
}

void MoveState::update(float dt)
{
    // Calculate some useful values
    glm::vec3 delta = target_ - unit_->pos_;
    float dist = glm::length(target_ - unit_->pos_);
    float desired_angle = 180.0f * atan2(delta.z , delta.x) / M_PI;
    float delAngle = desired_angle - unit_->angle_;
    float speed = getParam("unit.maxSpeed");
    float turnRate = getParam("unit.turnRate");
    // rotate
    // only rotate when not close enough
	if (dist > speed * dt)
    {
        // Get delta in [-180, 180]
        while (delAngle > 180.f) delAngle -= 360.f;
        while (delAngle < -180.f) delAngle += 360.f;
        // Would overshoot, just set angle
        if (fabs(delAngle) < turnRate * dt)
            unit_->angle_ = desired_angle;
        else
        {
            unit_->logger_->debug() << "delangle: " << delAngle << '\n';
            unit_->angle_ += glm::sign(delAngle) * turnRate * dt;
            while (unit_->angle_ > 360.f) unit_->angle_ -= 360.f;
            while (unit_->angle_ < 0.f) unit_->angle_ += 360.f;

            unit_->vel_ = glm::vec3(0.f);
        }
    }
    // move
    float rad = M_PI / 180.f * unit_->angle_;
    // And move, taking care to not overshoot
    glm::vec3 dir = glm::vec3(cosf(rad), 0, sinf(rad)); 
    if (dist < speed * dt)
        speed = dist / dt;
    unit_->vel_ = speed * dir;
}

void MoveState::stop()
{
    unit_->vel_ = glm::vec3(0.f);
}

UnitState * MoveState::next()
{
    glm::vec3 diff = target_ - unit_->pos_;
    float dist = glm::length(diff);

    if (dist < unit_->getRadius() / 2.f)
        return new NullState(unit_);
    return NULL;
}

void MoveState::serialize(Json::Value &obj)
{
    obj["target"] = toJson(target_);
}
