#include "rts/NetPlayer.h"
#include "common/NetConnection.h"
#include "common/util.h"
#include "rts/Game.h"
#include "rts/PlayerAction.h"

namespace rts {

NetPlayer::NetPlayer(id_t playerID, id_t teamID, const std::string &name,
    const glm::vec3 &color, NetConnectionPtr conn)
  : Player(playerID, teamID, name, color),
    connection_(conn),
    localPlayerID_(-1),
    ready_(false) {
  assert(connection_);
}

NetPlayer::~NetPlayer() {
  // conn goes out of scope and is automatically stopped/deleted
}

void NetPlayer::startTick(tick_t tick) {
  // nop
}

bool NetPlayer::isReady() const {
  if (ready_) {
    return true;
  }

  if (!connection_->running()) {
    PlayerAction a;
    a["type"] = ActionTypes::LEAVE_GAME;
    actionBuffer_.push_back(a);
    return ready_ = true;
  }

  std::unique_lock<std::mutex> lock(connection_->getMutex());
  std::queue<PlayerAction> &incomingActions = connection_->getQueue();
  while (!incomingActions.empty()) {
    PlayerAction a = incomingActions.front();
    incomingActions.pop();

    actionBuffer_.push_back(a);

    // if we get a none, then we're done with another frame... make it so
    if (a["type"] == ActionTypes::DONE) {
      return ready_ = true;
    }
  }

  return ready_;
}


std::vector<PlayerAction> NetPlayer::getActions() {
  invariant(ready_, "Asked for update when not ready");
  // reset, consume actions and ready state
  std::vector<PlayerAction> ret;
  ret.swap(actionBuffer_);
  ready_ = false;

  return ret;
}

void NetPlayer::playerAction(id_t playerID, const PlayerAction &action) {
  invariant(
    playerID == localPlayerID_,
    "Unexpected message from nonlocal player");

  connection_->sendPacket(action);
  // If the action was a leave game, close down thread
  if (action["type"] == ActionTypes::LEAVE_GAME) {
    connection_->stop();
  }
}
};  // rts
