#include "rts/MessageHub.h"
#include <cassert>
#include "rts/Game.h"

namespace rts {

MessageHub * MessageHub::instance = nullptr;

MessageHub * MessageHub::get() {
  if (!instance) {
    instance = new MessageHub();
  }
  return instance;
}

MessageHub::MessageHub()
  : game_(nullptr) {
}

MessageHub::~MessageHub() {
}

void MessageHub::setGame(Game *game) {
  game_ = game;
}

void MessageHub::addAction(const PlayerAction &action) {
  invariant(game_, "unset game object");
  invariant(action.isMember("pid"), "missing player id in action");
  invariant(action.isMember("type"), "missing type in action");

  game_->addAction(toID(action["pid"]), action);
}

void MessageHub::sendMessage(const Message &msg) {
  invariant(game_, "unset game object");
  invariant(msg.isMember("to"), "missing to field in message");
  invariant(msg.isMember("from"), "missing from field in message");

  Json::Value to = msg["to"];

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
    assertEid(eid);
    game_->sendMessage(eid, rlmsg);
  }
}
};  // rts
