#include "rts/GameEntity.h"
#include "common/Checksum.h"
#include "common/Collision.h"
#include "common/ParamReader.h"
#include "common/util.h"
#include "rts/Game.h"
#include "rts/Player.h"

namespace rts {

GameEntity::GameEntity(
    id_t id,
    const std::string &name,
    const Json::Value &params)
  : ModelEntity(id),
    playerID_(NO_PLAYER),
    name_(name),
    maxSpeed_(0.f),
    sight_(0.f),
    warp_(false) {
  setProperty(P_RENDERABLE, true);

  if (params.isMember("pid")) {
    playerID_ = assertPid(toID(params["pid"]));
  }
  if (params.isMember("pos")) {
    setPosition(glm::vec3(toVec2(params["pos"]), 0.f));
  }
  if (params.isMember("angle")) {
    setAngle(params["angle"].asFloat());
  }
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
  setSpeed(0.f);
  warp_ = false;
}

float GameEntity::distanceToEntity(const GameEntity *e) const {
  return rayBox2Intersection(
    getPosition2(),
    glm::normalize(e->getPosition2() - getPosition2()),
    e->getRect());
}

void GameEntity::remainStationary() {
  pathQueue_ = std::queue<glm::vec3>();
  setSpeed(0.f);
}

void GameEntity::turnTowards(const glm::vec2 &targetPos) {
  setAngle(angleToTarget(targetPos));
  setSpeed(0.f);
}

void GameEntity::moveTowards(const glm::vec2 &targetPos) {
  pathQueue_ = std::queue<glm::vec3>();
  pathQueue_.push(glm::vec3(targetPos, 0.f));
}

void GameEntity::warpPosition(const glm::vec2 &pos) {
  warp_ = true;
  warpTarget_ = pos;
}

void GameEntity::checksum(Checksum &chksum) const {
  id_t id = getID();
  chksum
    .process(id)
    .process(playerID_)
    .process(name_)
    .process(getPosition())
    .process(getAngle())
    .process(getSize())
    .process(getHeight())
    .process(getSpeed());
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
