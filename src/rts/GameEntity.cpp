#include "rts/GameEntity.h"
#include "common/Checksum.h"
#include "common/Collision.h"
#include "common/ParamReader.h"
#include "common/util.h"
#include "rts/Game.h"
#include "rts/Map.h"
#include "rts/Player.h"

namespace rts {

GameEntity::GameEntity(id_t id)
  : ModelEntity(id),
    playerID_(NO_PLAYER),
    gameID_(),
    maxSpeed_(0.f),
    sight_(0.f),
    warp_(false),
    uiInfo_() {
  resetTexture();
}

GameEntity::~GameEntity() {
}

void GameEntity::resetTexture() {
  const Player *player = Game::get()->getPlayer(getPlayerID());
  auto color = player ? player->getColor() : ::vec3Param("global.defaultColor");
  setColor(color);
}

Clock::time_point GameEntity::getLastTookDamage(uint32_t part) const {
  auto it = lastTookDamage_.find(part);
  if (it != lastTookDamage_.end()) {
    return it->second;
  }
  return Clock::time_point();
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

void GameEntity::setPlayerID(id_t pid) {
  assertPid(pid);
  playerID_ = pid;

  resetTexture();
}

void GameEntity::setTookDamage(int part_idx) {
  lastTookDamage_[part_idx] = Clock::now();
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

void GameEntity::resolve(float dt) {
	if (hasProperty(P_MOBILE) && warp_) {
		setPosition(warpTarget_);
	} else if (hasProperty(P_MOBILE) && !pathQueue_.empty() && getMaxSpeed() > 0) {
    glm::vec2 targetPos(pathQueue_.front());
    float dist = glm::length(targetPos - getPosition2());
    float speed = getMaxSpeed();
    // rotate
    turnTowards(targetPos);
    // move
    // Set speed careful not to overshoot
    if (dist < speed * dt) {
      speed = dist / dt;
      // 'pop'
      pathQueue_.erase(pathQueue_.begin());
    }
    setSpeed(speed);
  }
}

void GameEntity::checksum(Checksum &chksum) const {
  id_t id = getID();
  chksum
    .process(id)
    .process(playerID_)
    .process(getPosition())
    .process(getAngle())
    .process(getSize())
    .process(getHeight())
    .process(getSpeed());
}

std::vector<glm::vec3> GameEntity::getPathNodes() const {
  return pathQueue_;
}

void GameEntity::collide(const GameEntity *collider, float dt) {
  if (!hasProperty(P_MOBILE)) {
    return;
  }
  if (!collider->hasProperty(P_ACTOR)
      || collider->getPlayerID() != getPlayerID()) {
    return;
  }

  // if either entity is moving forward, just ignore the message
  if (getSpeed() != 0.f || collider->getSpeed() != 0.f) {
    return;
  }

  // get difference between positions at time of intersection
  auto diff = getPosition2(dt) - collider->getPosition2(dt);
  float overlap_dist = glm::length(diff);

  // seperate away, or randomly if exactly the same pos
  auto dir = (overlap_dist > 1e-6f)
    ? diff / overlap_dist
    : randDir2();

  float bumpSpeed = ::fltParam("global.bumpSpeed");
  addBumpVel(glm::vec3(dir * bumpSpeed, 0.f));
}
};  // rts
