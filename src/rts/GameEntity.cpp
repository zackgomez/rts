#include "rts/GameEntity.h"
#include "common/Checksum.h"
#include "common/Collision.h"
#include "common/ParamReader.h"
#include "common/util.h"
#include "rts/Game.h"
#include "rts/Map.h"
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

float GameEntity::distanceToEntity(const GameEntity *e) const {
  return e->distanceFromPoint(getPosition2());
}

void GameEntity::remainStationary() {
  warp_ = false;
  pathQueue_ = std::vector<glm::vec3>();
  lastTargetPos_ = glm::vec2(HUGE_VAL);
  setSpeed(0.f);
}

void GameEntity::turnTowards(const glm::vec2 &targetPos) {
  warp_ = false;
  setAngle(angleToTarget(targetPos));
  lastTargetPos_ = glm::vec2(HUGE_VAL);
  setSpeed(0.f);
}

void GameEntity::moveTowards(const glm::vec2 &targetPos) {
  warp_ = false;
  if (hasProperty(P_COLLIDABLE)) {
    if (targetPos != lastTargetPos_) {
      pathQueue_ = Game::get()->getMap()->getNavMesh()->getPath(
          getPosition(),
          glm::vec3(targetPos, 0));
      lastTargetPos_ = targetPos;
    }
  } else {
    pathQueue_.clear();
    pathQueue_.push_back(glm::vec3(targetPos, 0.f));
  }
}

void GameEntity::warpPosition(const glm::vec2 &pos) {
  warp_ = true;
  warpTarget_ = pos;
  setSpeed(0.f);
  lastTargetPos_ = glm::vec2(HUGE_VAL);
  pathQueue_ = std::vector<glm::vec3>();
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

std::vector<glm::vec3> GameEntity::getPathNodes() const {
  return pathQueue_;
}
};  // rts
