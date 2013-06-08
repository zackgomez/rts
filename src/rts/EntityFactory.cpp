#include "rts/EntityFactory.h"
#include "common/ParamReader.h"
#include "rts/Actor.h"
#include "rts/CollisionObject.h"

namespace rts {

EntityFactory::EntityFactory() {
}

EntityFactory::~EntityFactory() {
}

EntityFactory * EntityFactory::get() {
  static EntityFactory instance;
  return &instance;
}

GameEntity * EntityFactory::construct(rts::id_t id, 
    const std::string &name, const Json::Value &params) {
	return new Actor(id, name, params);
}
};  // namespace rts
