#include "Unit.h"
#include "ParamReader.h"
#include "math.h"

LoggerPtr unitLogger;
bool reached_;

Unit::Unit(int64_t playerID) :
    Entity(playerID),
    state_(new NullState(this))
{
    pos_ = glm::vec3(0, 0.5f, 0);
    radius_ = 0.5f;
    angle_ = 0;
    desired_angle_ = 0;
    dir_ = glm::vec2(cos(angle_*M_PI/180),
					 sin(angle_*M_PI/180));

    unitLogger = Logger::getLogger("Unit");

    vel_ = glm::vec3(0.f);
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

    dir_ = glm::vec2(cos(angle_*M_PI/180),
    				 sin(angle_*M_PI/180));
    pos_ += glm::vec3(vel_.x * dir_.x, 0.f, vel_.z * dir_.y);

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
    reached_ = false;
    unit_->vel_ = unit_->maxSpeed_ * (target_ - unit_->getPosition());
    glm::vec3 diff = target_ - unit_->pos_;
    unit_->desired_angle_ = 180.0f * atan2(diff.z , diff.x) / M_PI;
    unit_->vel_ = glm::vec3(0.05f);
}

void MoveState::update(float dt)
{
	if (reached_)
		return;

	float radius = unit_->getRadius();
	glm::vec3 diff = target_ - unit_->pos_;
	if (fabs(diff.x) <= radius &&
		fabs(diff.y) <= radius &&
		fabs(diff.z) <= radius)
	{
		unit_->vel_ = glm::vec3(0.f);
		reached_ = true;
	}

	else if (abs(unit_->desired_angle_ - unit_->angle_) > 2)
	{
		unit_->angle_ += (unit_->desired_angle_ - unit_->angle_)/30;
		unit_->vel_ = glm::vec3(0.f);
	}

	else
	{
		unit_->vel_ = glm::vec3(0.05f);
	}
}

UnitState * MoveState::next()
{
    return NULL;
}
