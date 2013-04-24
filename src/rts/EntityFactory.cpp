#include "rts/EntityFactory.h"
#include "common/ParamReader.h"
#include "rts/Building.h"
#include "rts/CollisionObject.h"
#include "rts/Projectile.h"
#include "rts/Unit.h"

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

  auto cl = strParam(name + ".class");

  // TODO(zack) use map
  if (cl == "Unit") {
    return new Unit(id, name, params);
  } else if (cl == "Projectile") {
    return new Projectile(id, name, params);
  } else if (cl == "Building") {
    return new Building(id, name, params);
  } else if (cl == "Collision") {
    return new CollisionObject(id, name, params);
  } else {
    LOG(WARNING) << "Trying to spawn unknown class " << cl
      << " named " << name << " params: " << params << '\n';
    return nullptr;
  }
}
};  // namespace rts
