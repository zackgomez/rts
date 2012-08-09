#include "MessageHub.h"
#include <cassert>
#include "Game.h"

namespace rts {

MessageHub * MessageHub::instance = NULL;

MessageHub * MessageHub::get() {
  if (!instance) {
    instance = new MessageHub();
  }
  return instance;
}

MessageHub::MessageHub()
  : game_(NULL) {
  logger_ = Logger::getLogger("MessageHub");
}

MessageHub::~MessageHub() {
}

void MessageHub::setGame(Game *game) {
  game_ = game;
}

const Entity * MessageHub::getEntity(id_t eid) const {
  return game_->getEntity(eid);
}

void MessageHub::sendMessage(const Message &msg) {
  invariant(game_, "unset game object");
  invariant(msg.isMember("to"), "missing to field in message");
  invariant(msg.isMember("from"), "missing from field in message");

  Json::Value to = msg["to"];

  // TODO(zack): force `to` to array here (see T61)

  // Force `to` as array
  if (!to.isArray()) {
    to = Json::Value(Json::arrayValue);
    to.append(msg["to"]);
  }
  invariant(to.isArray(), "'to' field must be an array");

  // Message with `to` as array for easy renderer consumption
  Message rlmsg = msg;
  rlmsg["to"] = to;

  // If an array, send to each member
  for (int i = 0; i < rlmsg["to"].size(); i++) {
    id_t eid = toID(rlmsg["to"][i]);
    if (eid == GAME_ID) {
      game_->handleMessage(rlmsg);
    } else {
      game_->sendMessage(eid, rlmsg);
    }
  }
}

void MessageHub::sendRemovalMessage(const Entity *e) {
  assert(e);

  Message msg;
  msg["to"] = toJson(GAME_ID);
  msg["from"] = toJson(e->getID());
  msg["type"] = MessageTypes::DESTROY_ENTITY;
  msg["eid"] = toJson(e->getID());

  sendMessage(msg);
}

}; // rts
