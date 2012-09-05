#include "rts/Entity.h"
#include "common/ParamReader.h"
#include "common/util.h"
#include "rts/MessageHub.h"
#include "rts/Player.h"

namespace rts {

id_t Entity::lastID_ = STARTING_EID;

Entity::Entity(const std::string &name,
               const Json::Value &params,
               bool mobile, bool targetable)
  : id_(lastID_++),
    playerID_(NO_PLAYER),
    name_(name),
    mobile_(mobile),
    targetable_(targetable),
    pos_(HUGE_VAL),
    angle_(0.f),
    radius_(0.f),
    height_(0.f),
    speed_(0.f),
    turnSpeed_(0.f) {
  if (params.isMember("entity_pid")) {
    playerID_ = toID(params["entity_pid"]);
  }
  if (params.isMember("entity_pos")) {
    pos_ = toVec3(params["entity_pos"]);
  }
  if (params.isMember("entity_angle")) {
    angle_ = params["entity_angle"].asFloat();
  }
  radius_ = param("radius");
  height_ = param("height");

  // Make sure we did it right
  assertEid(id_);
  assertPid(playerID_);
}

Entity::~Entity() {
}

id_t Entity::getTeamID() const {
  // No player, no team
  if (playerID_ == NO_PLAYER) {
    return NO_TEAM;
  }
  // A bit inefficient but OK for now
  return Game::get()->getPlayer(playerID_)->getTeamID();
}

void Entity::update(float dt) {
  if (mobile_) {
    // rotate
    angle_ += turnSpeed_ * dt;
    // clamp to [0, 360]
    while (angle_ > 360.f) {
      angle_ -= 360.f;
    }
    while (angle_ < 0.f) {
      angle_ += 360.f;
    }

    // move
    glm::vec3 vel = speed_ * getDirection(angle_);
    pos_ += vel * dt;
  }
}

glm::vec3 Entity::getPosition(float dt) const {
  if (!mobile_) {
    return getPosition();
  }

glm::vec3 vel = speed_ * getDirection(getAngle(dt));
  return pos_ + vel * dt;
}

float Entity::getAngle(float dt) const {
  if (!mobile_) {
    return getAngle();
  }
  return angle_ + turnSpeed_ * dt;
}

const glm::vec3 Entity::getDirection(float angle) {
  float rad = deg2rad(angle);
  return glm::vec3(cosf(rad), sinf(rad), 0);
}

const glm::vec3 Entity::getDirection() const {
  return getDirection(angle_);
}

float Entity::angleToTarget(const glm::vec3 &target) const {
  glm::vec3 delta = target - pos_;
  return rad2deg(atan2(delta.y , delta.x));
}

float Entity::distanceBetweenEntities(const Entity *e) const {
  glm::vec3 targetPos = e->getPosition();
  return glm::length(targetPos - pos_);
}

float Entity::param(const std::string &p) const {
  return fltParam(name_ + "." + p);
}

std::string Entity::strParam(const std::string &p) const {
  return ::strParam(name_ + "." + p);
}

bool Entity::hasParam(const std::string &p) const {
  return ParamReader::get()->hasFloat(name_ + "." + p);
}

bool Entity::hasStrParam(const std::string &p) const {
  return ParamReader::get()->hasString(name_ + "." + p);
}
};  // rts
