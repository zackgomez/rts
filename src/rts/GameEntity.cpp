#include "rts/GameEntity.h"
#include "common/Checksum.h"
#include "common/Collision.h"
#include "common/ParamReader.h"
#include "common/util.h"
#include "rts/MessageHub.h"
#include "rts/Player.h"

namespace rts {

GameEntity::GameEntity(
    const std::string &name,
    const Json::Value &params,
    bool mobile, bool targetable,
    bool collidable)
  : playerID_(NO_PLAYER),
    name_(name),
    mobile_(mobile),
    targetable_(targetable),
    collidable_(collidable),
    pos_(HUGE_VAL),
    angle_(0.f),
    size_(glm::vec2(0.f)),
    height_(0.f),
    speed_(0.f),
    turnSpeed_(0.f) {
  if (params.isMember("entity_pid")) {
    playerID_ = toID(params["entity_pid"]);
  }
  if (params.isMember("entity_pos")) {
    pos_ = toVec2(params["entity_pos"]);
  }
  if (params.isMember("entity_size")) {
    size_ = toVec2(params["entity_size"]);
  } else {
    size_ = vec2Param("size");
  }
  if (params.isMember("entity_angle")) {
    angle_ = params["entity_angle"].asFloat();
  }
  if (hasParam("height"))
    height_ = param("height");

  // Make sure we did it right
  assertPid(playerID_);
}

GameEntity::~GameEntity() {
}

id_t GameEntity::getTeamID() const {
  // No player, no team
  if (playerID_ == NO_PLAYER) {
    return NO_TEAM;
  }
  // A bit inefficient but OK for now
  return Game::get()->getPlayer(playerID_)->getTeamID();
}

void GameEntity::update(float dt) {
  turnSpeed_ = 0.f;
  speed_ = 0.f;
}

void GameEntity::integrate(float dt) {
  // for immobile entities, just check that they are stationary
  if (!mobile_) {
    invariant(
      turnSpeed_ == 0.f && speed_ == 0.f,
      "Immobile Entities shouldn't have motion");
    return;
  }

  // TODO(zack): upgrade to a better integrator (runge-kutta)
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
  glm::vec2 vel = speed_ * getDirection(angle_);
  pos_ += vel * dt;
}

void GameEntity::handleMessage(const Message &msg) {
  if (msg["type"] == MessageTypes::COLLISION) {
    invariant(collidable_, "Got collision for noncollidable object!");
  } else {
    LOG(INFO) << "Entity of type " << getType()
      << " received unknown message type: " << msg["type"] << '\n';
  }
}

void GameEntity::checksum(Checksum &chksum) const {
  id_t id = getID();
  chksum.process(&id, sizeof(id))
      .process(&playerID_, sizeof(playerID_))
      .process(&name_[0], name_.length())
      .process(&mobile_, sizeof(mobile_))
      .process(&targetable_, sizeof(targetable_))
      .process(&pos_, sizeof(pos_))
      .process(&angle_, sizeof(angle_))
      .process(&size_, sizeof(size_))
      .process(&height_, sizeof(height_))
      .process(&speed_, sizeof(speed_))
      .process(&turnSpeed_, sizeof(turnSpeed_));
}

glm::vec2 GameEntity::getPosition(float dt) const {
  if (!mobile_) {
    return getPosition();
  }

  glm::vec2 vel = speed_ * getDirection(getAngle(dt));
  return pos_ + vel * dt;
}

float GameEntity::getAngle(float dt) const {
  if (!mobile_) {
    return getAngle();
  }
  return angle_ + turnSpeed_ * dt;
}

const glm::vec2 GameEntity::getDirection(float angle) {
  float rad = deg2rad(angle);
  return glm::vec2(cosf(rad), sinf(rad));
}

const glm::vec2 GameEntity::getDirection() const {
  return getDirection(angle_);
}

float GameEntity::angleToTarget(const glm::vec2 &target) const {
  glm::vec2 delta = target - pos_;
  return rad2deg(atan2(delta.y , delta.x));
}

float GameEntity::distanceBetweenEntities(const GameEntity *e) const {
  glm::vec2 targetPos = e->getPosition();
  return glm::length(targetPos - pos_);
}

bool GameEntity::pointInEntity(const glm::vec2 &p) {
  return pointInBox(p, pos_, size_, angle_);
}

float GameEntity::param(const std::string &p) const {
  return fltParam(name_ + "." + p);
}

std::string GameEntity::strParam(const std::string &p) const {
  return ::strParam(name_ + "." + p);
}

glm::vec2 GameEntity::vec2Param(const std::string &p) const {
  return ::vec2Param(name_ + "." + p);
}

bool GameEntity::hasParam(const std::string &p) const {
  return ::hasParam(name_ + "." + p);
}
};  // rts
