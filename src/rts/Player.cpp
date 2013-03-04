#include "rts/Player.h"
#include <SDL/SDL.h>
#include "common/Checksum.h"
#include "common/ParamReader.h"
#include "rts/Building.h"
#include "rts/GameEntity.h"
#include "rts/Game.h"
#include "rts/MessageHub.h"
#include "rts/Renderer.h"

namespace rts {

LocalPlayer::LocalPlayer(id_t playerID, id_t teamID, const std::string &name,
    const glm::vec3 &color)
  : Player(playerID, teamID, name, color) {
}

LocalPlayer::~LocalPlayer() {
}

void LocalPlayer::startTick(tick_t tick) {
  PlayerAction a;
  a["type"] = ActionTypes::DONE;
  a["pid"] = toJson(playerID_);
  a["checksum"] = game_->getChecksum().toJson();
  a["tick"] = toJson(tick);
  game_->addAction(playerID_, a);
}

bool LocalPlayer::isReady() const {
  return true;
}

std::vector<PlayerAction> LocalPlayer::getActions() {
  // Collect the actions for this tick
  std::unique_lock<std::mutex> lock(actionMutex_);
  std::vector<PlayerAction> ret;
  for (;;) {
    invariant(!actions_.empty(), "Unexpectedly ran out of actions!");
    auto action = actions_.front();
    actions_.pop();
    ret.push_back(action);
    if (action["type"] == ActionTypes::DONE) {
      break;
    }
  }

  return ret;
}

void LocalPlayer::playerAction(id_t playerID, const PlayerAction &action) {
  if (playerID == playerID_) {
    std::unique_lock<std::mutex> lock(actionMutex_);
    actions_.push(action);
  }
}

DummyPlayer::DummyPlayer(id_t playerID, id_t teamID)
  : Player(playerID, teamID, "DummyPlayer", vec3Param("colors.dummyPlayer")) {
}

void DummyPlayer::startTick(tick_t tick) {
  PlayerAction action;
  action["type"] = ActionTypes::DONE;
  action["pid"] = toJson(playerID_);
  action["checksum"] = game_->getChecksum().toJson();
  action["tick"] = toJson(tick);
  actions_.push(action);
}

std::vector<PlayerAction> DummyPlayer::getActions() {
  invariant(!actions_.empty(), "No actions!");
  std::vector<PlayerAction> ret;
  ret.push_back(actions_.front());
  actions_.pop();

  return ret;
}
};  // rts
