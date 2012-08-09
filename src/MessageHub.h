#pragma once
#include "Logger.h"
#include "Entity.h"
#include "Message.h"
#include "Game.h"
#include "util.h"

namespace rts {

class MessageHub {
public:
  static MessageHub* get();
  ~MessageHub();

  void setGame(Game *game);

  const Entity *getEntity(id_t eid) const;
  // Immediately dispatches message
  void sendMessage(const Message &msg);

  void sendRemovalMessage(const Entity *e);

  // Returns the entity that scores the LOWEST with the given scoring function.
  // Scoring function should have signature float scorer(const Entity *);
  template <class T>
  const Entity *findEntity(T scorer) const {
    invariant(game_, "unset game object");
    return game_->findEntity(scorer);
  }
  // TODO(zack) a function that returns a list of entities similar to the
  // above function

private:
  // For singleton
  MessageHub();
  static MessageHub *instance;
  Game *game_;
  LoggerPtr logger_;
};
}; // rts
