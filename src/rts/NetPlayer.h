#ifndef SRC_RTS_NETPLAYER_H_
#define SRC_RTS_NETPLAYER_H_

#include "Player.h"
#include "NetConnection.h"

namespace rts {

class NetPlayer : public Player {
 public:
  NetPlayer(id_t playerID, id_t teamID, const std::string &name,
            const glm::vec3 &color, NetConnection *conn);
  virtual ~NetPlayer();

  virtual bool update(tick_t tick);
  virtual void playerAction(id_t playerID, const PlayerAction &action);

  void setLocalPlayer(id_t playerID) {
    localPlayerID_ = playerID;
  }

 private:
  LoggerPtr logger_;
  NetConnection *connection_;
  std::mutex mutex_;
  // last tick that is fully received
  tick_t doneTick_;

  // The id of the local player who's actions we want to send
  id_t localPlayerID_;
};
};  // rts

#endif  // SRC_RTS_NETPLAYER_H_
