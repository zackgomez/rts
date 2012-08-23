#include <string>
#include <iostream>
#include "Building.h"
#include "ParamReader.h"
#include "MessageHub.h"
#include "Unit.h"

namespace rts {

LoggerPtr Building::logger_;
const std::string Building::TYPE = "BUILDING";

Building::Building(const std::string &name, const Json::Value &params) :
  Actor(name, params),
  capAmount_(0.f),
  capperID_(NO_ENTITY),
  lastCappingPlayerID_(NO_PLAYER),
  capResetDelay_(0) {
}

void Building::handleMessage(const Message &msg) {
  if (msg["type"] == MessageTypes::CAPTURE) {
    invariant(msg.isMember("pid"), "malformed capture message");
    invariant(msg.isMember("cap"), "malformed attack message");
    invariant(toID(msg["pid"]) != getPlayerID(), 
        "cannot capture own structures");
    invariant(getType() == Building::TYPE, "can only capture structures");
    invariant(isCappable(), "structure not capturable");

    // Tells us the unit is still capping the point.
    capResetDelay_ = 0;

    capperID_ = toID(msg["from"]);
    if (getPlayerID() == NO_PLAYER) {
      if (lastCappingPlayerID_ == toID(msg["pid"])) {
        capAmount_ += msg["cap"].asFloat();
        float max_cap = param("captureTime");
        if (capAmount_ >= max_cap) {
          setPlayerID(toID(msg["pid"]));
          capAmount_ = max_cap;
        }
      } else {
        capAmount_ = 0.f;
        lastCappingPlayerID_ = toID(msg["pid"]);
      }
    } else {
      capAmount_ -= msg["cap"].asFloat();
      if (capAmount_ <= 0.f) {
        setPlayerID(NO_PLAYER);
        capAmount_ *= -1;
      }
    }
  } else {
    Actor::handleMessage(msg);
  }
}

void Building::update(float dt) {
  Actor::update(dt);

  // We need a single tick delay to see if an entity is still capturing this
  // building. The single tick ensures that the message from the capping unit
  // would have been sent, so if no such message was sent, the unit is no
  // longer capping.
  capResetDelay_++;
  if (capResetDelay_ > 1) capperID_ = NO_ENTITY;

  if (getPlayerID() == NO_PLAYER) {
    if (capperID_ == NO_ENTITY) capAmount_ -= dt;
    if (capAmount_ < 0.f) capAmount_ = 0.f;
    return;
  }

  if (capperID_ == NO_ENTITY) capAmount_ += dt;
  if (capAmount_ > getMaxCap()) capAmount_ = getMaxCap();

  // Building generates resources
  if (hasParam("reqGen"))
    MessageHub::get()->sendResourceMessage(
      getID(),
      getPlayerID(),
      ResourceTypes::REQUISITION,
      param("reqGen") * dt
    );
  if (hasParam("victoryGen"))
    MessageHub::get()->sendResourceMessage(
      getID(),
      getPlayerID(),
      ResourceTypes::VICTORY,
      param("victoryGen") * dt
    );
}

bool Building::canCapture(id_t eid) const {
  const Entity *e = Game::get()->getEntity(eid);
  id_t pid = e->getPlayerID();
  return isCappable() && pid != getPlayerID() && 
    (capperID_ == NO_ENTITY || eid == capperID_);
}

bool Building::needsRemoval() const {
  return health_ <= 0.f;
}

}; // rts
