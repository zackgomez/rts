#include "rts/GameEntity.h"
#include "common/Checksum.h"
#include "common/Collision.h"
#include "common/ParamReader.h"
#include "common/util.h"
#include "rts/MessageHub.h"
#include "rts/Player.h"

namespace rts {

GameEntity::GameEntity(
    id_t id,
    const std::string &name,
    const Json::Value &params,
    bool targetable,
    bool collidable)
  : ModelEntity(id),
    playerID_(NO_PLAYER),
    name_(name),
    targetable_(targetable),
    collidable_(collidable) {
  if (params.isMember("entity_pid")) {
    playerID_ = toID(params["entity_pid"]);
  }
  if (params.isMember("entity_pos")) {
    setPosition(toVec2(params["entity_pos"]));
  }
  if (params.isMember("entity_size")) {
    setSize(toVec2(params["entity_size"]));
  } else {
    setSize(vec2Param("size"));
  }
  if (params.isMember("entity_angle")) {
    setAngle(params["entity_angle"].asFloat());
  }
  if (hasParam("height")) {
    setHeight(fltParam("height"));
  }

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
  setBumpVel(glm::vec3(0.f));
  setTurnSpeed(0.f);
  setSpeed(0.f);
}

void GameEntity::handleMessage(const Message &msg) {
  LOG(INFO) << "Entity named " << name_
    << " received unknown message type: " << msg["type"] << '\n';
}

void GameEntity::remainStationary() {
  pathQueue_ = std::queue<glm::vec3>();
  setSpeed(0.f);
  setTurnSpeed(0.f);
}

void GameEntity::turnTowards(const glm::vec2 &targetPos, float dt) {
  float desired_angle = angleToTarget(targetPos);
  float delAngle = addAngles(desired_angle, -getAngle());
  float turnRate = fltParam("turnRate");
  // rotate
  // only rotate when not close enough
  // Would overshoot, just move directly there
  if (fabs(delAngle) < turnRate * dt) {
    setTurnSpeed(delAngle / dt);
  } else {
    setTurnSpeed(glm::sign(delAngle) * turnRate);
  }
  // No movement
  setSpeed(0.f);
}

void GameEntity::moveTowards(const glm::vec2 &targetPos, float dt) {
  pathQueue_ = std::queue<glm::vec3>();
  pathQueue_.push(glm::vec3(targetPos, 0.f));
  float dist = glm::length(targetPos - getPosition2());
  float speed = fltParam("speed");
  // rotate
  turnTowards(targetPos, dt);
  // move
  // Set speed careful not to overshoot
  if (dist < speed * dt) {
    speed = dist / dt;
  }
  setSpeed(speed);
}

void GameEntity::checksum(Checksum &chksum) const {
  id_t id = getID();
  chksum
    .process(id)
    .process(playerID_)
    .process(name_)
    .process(targetable_)
    .process(getPosition())
    .process(getAngle())
    .process(getSize())
    .process(getHeight())
    .process(getSpeed())
    .process(getTurnSpeed());
}

std::queue<glm::vec3> GameEntity::getPathNodes() const {
  return pathQueue_;
}

Json::Value GameEntity::getParam(const std::string &p) const {
  return ::getParam(name_ + "." + p);
}

float GameEntity::fltParam(const std::string &p) const {
  return ::fltParam(name_ + "." + p);
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
