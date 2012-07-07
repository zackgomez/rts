#include "Unit.h"
#include "ParamReader.h"
#include "math.h"

LoggerPtr unitLogger;

Unit::Unit(int64_t playerID) :
    Entity(playerID),
    state_(new MoveState(glm::vec3(0.f), this))
{
    pos_ = glm::vec3(0, 0.5f, 0);
    radius_ = 0.5f;
    angle_ = 0;
    dir_ = glm::vec2(sin(angle_*M_PI/180),
					 cos(angle_*M_PI/180));

    unitLogger = Logger::getLogger("Unit");

    vel_ = glm::vec3(0.f);
    maxSpeed_ = getParam("unit.maxSpeed");
}

void Unit::update(float dt)
{
    state_->update(dt);

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
    unit_->vel_ = unit_->maxSpeed_ * (target_ - unit_->getPosition());
}

void MoveState::update(float dt)
{
	glm::vec3 diff = (target_ - unit_->pos_);
	if (abs(diff.x) < 0.05 &&
		abs(diff.y) < 0.05 &&
		abs(diff.z) < 0.05)
	{
		unit_->vel_ = glm::vec3(0.f);
	}
	else {
		unit_->angle_ = 180 * atan(abs(diff.z) / abs(diff.x)) / M_PI;
		if (diff.x < 0 && diff.z < 0)
			unit_->angle_ += 180;
		else if (diff.x < 0)
			unit_->angle_ += 90;
		else if (diff.z < 0)
			unit_->angle_ += 270;
		unit_->dir_ = glm::vec2(cos(unit_->angle_*M_PI/180),
							sin(unit_->angle_*M_PI/180));
		unit_->vel_ = glm::vec3(0.05f);
	}

	unitLogger->info() << unit_->angle_ << ": " << diff.x <<"\n";


}

UnitState * MoveState::next()
{
    return NULL;
}
