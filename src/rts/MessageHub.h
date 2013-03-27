#ifndef SRC_RTS_MESSAGEHUB_H_
#define SRC_RTS_MESSAGEHUB_H_

#include <utility>
#include "common/Logger.h"
#include "common/util.h"
#include "rts/GameEntity.h"
#include "rts/Message.h"
#include "rts/Game.h"

namespace rts {

class MessageHub {
 public:
  static MessageHub* get();
  ~MessageHub();

  void setGame(Game *game);

  void addAction(const PlayerAction &action);

  // Immediately dispatches message
  void sendMessage(const Message &msg);

  void sendSpawnMessage(id_t from, const std::string &eclass,
                        const std::string &ename, const Json::Value &eparams);
  void sendRemovalMessage(const GameEntity *e);
  void sendResourceMessage(id_t from, id_t pid, const std::string &resource,
      float amount);
  void sendVPMessage(id_t from, id_t tid, float amount);
  void sendCollisionMessage(id_t from, id_t to);

 private:
  // For singleton
  MessageHub();
  static MessageHub *instance;
  Game *game_;
};
};  // rts

#endif  // SRC_RTS_MESSAGEHUB_H_
