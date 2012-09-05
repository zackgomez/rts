#ifndef SRC_RTS_MESSAGEHUB_H_
#define SRC_RTS_MESSAGEHUB_H_

#include "common/Logger.h"
#include "common/util.h"
#include "rts/Entity.h"
#include "rts/Message.h"
#include "rts/Game.h"

namespace rts {

class MessageHub {
 public:
  static MessageHub* get();
  ~MessageHub();

  void setGame(Game *game);

  // Immediately dispatches message
  void sendMessage(const Message &msg);

  void sendSpawnMessage(id_t from, const std::string &eclass,
                        const std::string &ename, const Json::Value &eparams);
  void sendRemovalMessage(const Entity *e);
  void sendResourceMessage(id_t from, id_t pid, const std::string &resource,
      float amount);
  void sendVPMessage(id_t from, id_t tid, float amount);

  // Returns the entity that scores the LOWEST with the given scoring function.
  // Scoring function should have signature float scorer(const Entity *);
  // TODO(zack) REMOVE THIS FUNCTION and just use the Game:: version
  template <class T>
  const Entity *findEntity(T scorer) const {
    invariant(game_, "unset game object");
    return game_->findEntity(scorer);
  }

 private:
  // For singleton
  MessageHub();
  static MessageHub *instance;
  Game *game_;
  LoggerPtr logger_;
};
};  // rts

#endif  // SRC_RTS_MESSAGEHUB_H_
