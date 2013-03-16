#ifndef SRC_RTS_ENTITYFACTORY_H_
#define SRC_RTS_ENTITYFACTORY_H_

#include <string>
#include <json/json.h>
#include "common/Logger.h"
#include "common/Types.h"

namespace rts {

class GameEntity;

class EntityFactory {
 public:
  static EntityFactory *get();

  GameEntity * construct(id_t id, const std::string &cl,
                     const std::string &name, const Json::Value &params);

 private:
  EntityFactory();
  ~EntityFactory();

  LoggerPtr logger_;
};
};  // rts

#endif  // SRC_RTS_ENTITYFACTORY_H_
