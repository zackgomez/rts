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

    if (fabs(angle_ - desired_angle_) > 5) {
    	angle_ += (desired_angle_ - angle_)/30;
    	return;
    }

    dir_ = glm::vec2(cos(desired_angle_*M_PI/180),
    				 sin(desired_angle_*M_PI/180));
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
}

void MoveState::update(float dt)
{
	if (reached_)
		return;

	glm::vec3 diff = target_ - unit_->pos_;
	if (fabs(diff.x) < 0.5 &&
		fabs(diff.y) < 0.5 &&
		fabs(diff.z) < 0.5)
	{
		unit_->vel_ = glm::vec3(0.f);
		reached_ = true;
	}

	else if (target_ == prev_target_)
	{
		return;
	}

	else {
		float angle = 0;

		if (abs(diff.x) < 0.001)
			angle = 90.0f;
		else
			angle = 180.0f * atan(fabs(diff.z) / fabs(diff.x)) / M_PI;

		if (diff.x < 0 && diff.z < 0)
			angle += 180.0f;
		else if (diff.x < 0)
			angle = 180.0f - angle;
		else if (diff.z < 0)
			angle = 360.0f - angle;

		unit_->desired_angle_ = angle;
		unit_->vel_ = glm::vec3(0.05f);

		prev_target_ = target_;
	}

}

UnitState * MoveState::next()
{
    return NULL;
}
