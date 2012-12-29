#include "rts/EntityFactory.h"
#include "rts/Unit.h"
#include "rts/Projectile.h"
#include "rts/Building.h"
#include "rts/CollisionObject.h"

namespace rts {

EntityFactory::EntityFactory() {
  logger_ = Logger::getLogger("EntityFactory");
}

EntityFactory::~EntityFactory() {
}

EntityFactory * EntityFactory::get() {
  static EntityFactory instance;
  return &instance;
}

GameEntity * EntityFactory::construct(const std::string &cl,
    const std::string &name, const Json::Value &params) {
  // TODO(zack) use map
  if (cl == Unit::TYPE) {
    return new Unit(name, params);
  } else if (cl == Projectile::TYPE) {
    return new Projectile(name, params);
  } else if (cl == Building::TYPE) {
    return new Building(name, params);
  } else if (cl == CollisionObject::TYPE) {
    return new CollisionObject(name, params);
  } else {
    logger_->warning() << "Trying to spawn unknown class " << cl <<
                       " named " << name << " params: " << params << '\n';
    return nullptr;
  }
}
};  // namespace rts
