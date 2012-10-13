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
  logger_ = Logger::getLogger("MessageHub");
}

MessageHub::~MessageHub() {
}

void MessageHub::setGame(Game *game) {
  game_ = game;
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

void MessageHub::sendSpawnMessage(id_t from, const std::string &eclass,
    const std::string &ename, const Json::Value &eparams) {
  Message msg;
  msg["to"]   = toJson(GAME_ID);
  msg["from"] = toJson(from);
  msg["type"] = MessageTypes::SPAWN_ENTITY;
  msg["entity_class"] = eclass;
  msg["entity_name"]  = ename;
  msg["params"] = eparams;

  sendMessage(msg);
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

void MessageHub::sendResourceMessage(
    id_t from,
    id_t pid,
    const std::string &resource,
    float amount) {
  Message msg;
  msg["to"] = toJson(GAME_ID);
  msg["from"] = toJson(from);
  msg["type"] = MessageTypes::ADD_RESOURCE;
  msg["pid"] = toJson(pid);
  msg["resource"] = resource;
  msg["amount"] = amount;

  sendMessage(msg);
}

void MessageHub::sendVPMessage(
    id_t from,
    id_t tid,
    float amount) {
  Message msg;
  msg["to"] = toJson(GAME_ID);
  msg["from"] = toJson(from);
  msg["type"] = MessageTypes::ADD_VP;
  msg["tid"] = toJson(tid);
  msg["amount"] = amount;

  sendMessage(msg);
}

void MessageHub::sendCollisionMessage(id_t from, id_t to) {
  Message msg;
  msg["to"] = toJson(to);
  msg["from"] = toJson(from);
  msg["type"] = MessageTypes::COLLISION;

  sendMessage(msg);
}
};  // rts
