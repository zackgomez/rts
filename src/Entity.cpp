#include "Entity.h"
#include "ParamReader.h"

namespace rts {

const id_t Entity::STARTING_ID = 100;
id_t Entity::lastID_ = Entity::STARTING_ID;

Entity::Entity(const std::string &name,
    const Json::Value &params,
    bool mobile, bool targetable) :
  id_(lastID_++),
  playerID_(NO_PLAYER),
  name_(name),
  mobile_(mobile),
  targetable_(targetable),
  pos_(HUGE_VAL),
  angle_(0.f),
  radius_(0.f),
  speed_(0.f),
  turnSpeed_(0.f)
{
  if (params.isMember("entity_pid")) playerID_ = params["entity_pid"].asUInt64();
  if (params.isMember("entity_pos")) pos_ = toVec3(params["entity_pos"]);
  if (params.isMember("entity_angle")) angle_ = params["entity_angle"].asFloat();
  radius_ = param("radius");
}

Entity::~Entity()
{
}

void Entity::update(float dt)
{
  if (mobile_)
  {
    // rotate
    angle_ += turnSpeed_ * dt;
    // clamp to [0, 360]
    while (angle_ > 360.f) angle_ -= 360.f;
    while (angle_ < 0.f) angle_ += 360.f;

    // move
    glm::vec3 vel = speed_ * getDirection(angle_);
    pos_ += vel * dt;
  }
}

glm::vec3 Entity::getPosition(float dt) const
{
  if (!mobile_)
    return getPosition();

  glm::vec3 vel = speed_ * getDirection(getAngle(dt));
  return pos_ + vel * dt;
}

float Entity::getAngle(float dt) const
{
  if (!mobile_)
    return getAngle();
  return angle_ + turnSpeed_ * dt;
}

const glm::vec3 Entity::getDirection(float angle)
{
  float rad = deg2rad(angle);
  return glm::vec3(cosf(rad), 0, sinf(rad)); 
}

const glm::vec3 Entity::getDirection() const
{
  return getDirection(angle_);
}

float Entity::angleToTarget(const glm::vec3 &target) const
{
  glm::vec3 delta = target - pos_;
  return rad2deg(atan2(delta.z , delta.x));
}

float Entity::param(const std::string &p) const
{
  return getParam(name_ + "." + p);
}

};
