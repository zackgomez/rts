#include "rts/Building.h"
#include <string>
#include <iostream>
#include "common/ParamReader.h"
#include "rts/MessageHub.h"
#include "rts/Player.h"
#include "rts/ResourceManager.h"
#include "rts/Unit.h"

namespace rts {

Building::Building(id_t id, const std::string &name, const Json::Value &params)
  : Actor(id, name, params) {
}

bool Building::canCapture(id_t eid) const {
  const GameEntity *e = Game::get()->getEntity(eid);
  id_t pid = e->getPlayerID();
  // TODO(zack): bring this back
  return hasProperty(P_CAPPABLE) && pid != getPlayerID();
    // (capperID_ == NO_ENTITY || eid == capperID_);
}
};  // rts
