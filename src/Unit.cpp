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
    angle_ = 0;
    dir_ = glm::vec2(sin(angle_*M_PI/180),
					 cos(angle_*M_PI/180));

    unitLogger = Logger::getLogger("Unit");

    vel_ = glm::vec3(0.1f);
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
}

void Unit::update(float dt)
{
    state_->update(dt);

    pos_ += glm::vec3(vel_.x * cos (angle_ * M_PI / 180), 0.f,
					  vel_.z * sin (angle_ * M_PI / 180));

}

bool Unit::needsRemoval() const
{
    return false;
}

void Unit::serialize(Json::Value &obj) const
{
    // TODO
}

MoveState::MoveState(const glm::vec3 &target, Unit *unit) :
    UnitState(unit),
    target_(target)
{
    // Make sure the unit stands on the terrain
    target_.y += unit_->getRadius(); 
    unit_->vel_ = unit_->maxSpeed_ * (target_ - unit_->getPosition());
}

void MoveState::update(float dt)
{
}

UnitState * MoveState::next()
{
    return NULL;
}
