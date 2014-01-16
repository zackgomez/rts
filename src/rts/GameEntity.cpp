#include "rts/GameEntity.h"
#include "common/Checksum.h"
#include "common/Collision.h"
#include "common/ParamReader.h"
#include "common/util.h"
#include "rts/Game.h"
#include "rts/Map.h"
#include "rts/Player.h"

static const uint32_t P_GAMEENTITY = 293013864;

namespace rts {

template<>
GameEntity::UIInfo curve_sample<GameEntity::UIInfo>::interpolateFrom(float interpt, const curve_sample<GameEntity::UIInfo> &s0) {
  float u = (interpt - s0.t) / (t - s0.t);

  GameEntity::UIInfo ret = s0.val;
  ret.mana = val.mana * u + s0.val.mana * (1.f - u);
  return ret;
}

GameEntity* GameEntity::cast(ModelEntity *e) {
  if (!e) {
    return nullptr;
  }
  if (!e->hasProperty(P_GAMEENTITY)) {
    return nullptr;
  }
  return (GameEntity *)e;
}

const GameEntity* GameEntity::cast(const ModelEntity *e) {
  return GameEntity::cast((ModelEntity*) e);
}

GameEntity::GameEntity(id_t id) : ModelEntity(id),
    gameID_(),
    playerCurve_(NO_PLAYER),
    teamCurve_(NO_PLAYER),
    aliveCurve_(false),
    uiInfoCurve_(UIInfo()),
    visibilityCurve_(VisibilitySet()),
    sight_(0.f) {
}

GameEntity::~GameEntity() {
}

bool GameEntity::hasProperty(uint32_t property) const {
  if (property == P_GAMEENTITY) {
    return true;
  }
  return properties_.count(property);
}

void GameEntity::preRender(float t) {
  const Player *player = Game::get()->getPlayer(getPlayerID(t));
  auto color = player ? player->getColor() : ::vec3Param("global.defaultColor");
  setColor(color);

  bool visible = isVisible() && getAlive(t);
  setVisible(visible);
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

id_t GameEntity::getPlayerID(float t) const {
  return playerCurve_.stepSample(t);
}

id_t GameEntity::getTeamID(float t) const {
  return teamCurve_.stepSample(t);
}

bool GameEntity::getAlive(float t) const {
  return aliveCurve_.stepSample(t);
}

GameEntity::UIInfo GameEntity::getUIInfo(float t) const {
return uiInfoCurve_.linearSample(t);
}

void GameEntity::setPlayerID(float t, id_t pid) {
  assertPid(pid);
  playerCurve_.addKeyframe(t, pid);
}

void GameEntity::setTeamID(float t, id_t tid) {
  assertTid(tid);
  teamCurve_.addKeyframe(t, tid);
}

void GameEntity::setAlive(float t, bool alive) {
  aliveCurve_.addKeyframe(t, alive);
}

void GameEntity::setUIInfo(float t, const UIInfo &ui_info) {
  uiInfoCurve_.addKeyframe(t, ui_info);
}

void GameEntity::setTookDamage(int part_idx) {
  lastTookDamage_[part_idx] = Clock::now();
}

GameEntity::UIInfo::UIInfo()
  : mana(0.f), retreat(false), capture(0.f), capture_pid(NO_PLAYER), hotkey('\0') {
}
};  // rts
