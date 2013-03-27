#ifndef SRC_RTS_NETPLAYER_H_
#define SRC_RTS_NETPLAYER_H_

#include "rts/Player.h"
#include <mutex>
#include "common/NetConnection.h"

namespace rts {

class NetPlayer : public Player {
 public:
  NetPlayer(id_t playerID, id_t teamID, const std::string &name,
            const glm::vec3 &color, NetConnectionPtr conn);
  virtual ~NetPlayer();

  virtual void startTick(tick_t tick);
  virtual bool isReady() const;
  virtual std::vector<PlayerAction> getActions();
  virtual void playerAction(id_t playerID, const PlayerAction &action);

  void setLocalPlayer(id_t playerID) {
    localPlayerID_ = playerID;
  }

 private:
  NetConnectionPtr connection_;
  std::mutex mutex_;
  // last tick that is fully received
  tick_t doneTick_;

  // The id of the local player who's actions we want to send
  id_t localPlayerID_;

  mutable bool ready_;
  mutable std::vector<PlayerAction> actionBuffer_;
};
};  // rts

#endif  // SRC_RTS_NETPLAYER_H_
