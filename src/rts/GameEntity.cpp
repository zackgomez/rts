#include "rts/GameEntity.h"
#include "common/Checksum.h"
#include "common/Collision.h"
#include "common/ParamReader.h"
#include "common/util.h"
#include "rts/Game.h"
#include "rts/Map.h"
#include "rts/Player.h"

namespace rts {

GameEntity::GameEntity(id_t id) : ModelEntity(id),
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

void GameEntity::setPlayerID(id_t pid) {
  assertPid(pid);
  playerID_ = pid;

  resetTexture();
}

void GameEntity::setTookDamage(int part_idx) {
  lastTookDamage_[part_idx] = Clock::now();
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
};  // rts
