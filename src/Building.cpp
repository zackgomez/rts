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
  Actor(name, params) {
}

void Building::handleMessage(const Message &msg) {
  Actor::handleMessage(msg);
}

void Building::update(float dt) {
  Actor::update(dt);

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

bool Building::needsRemoval() const {
  return health_ <= 0.f;
}

}; // rts
