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
    sight_(0.f),
    visibilityCurve_(VisibilitySet()),
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

bool GameEntity::isVisibleTo(float t, id_t pid) const {
  const auto& visibility_set = visibilityCurve_.stepSample(t);
  return visibility_set.find(pid) != visibility_set.end();
}

void GameEntity::setVisibilitySet(float t, const VisibilitySet &flags) {
  visibilityCurve_.addKeyframe(t, flags);
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
};  // rts
