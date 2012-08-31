#ifndef SRC_RTS_ENTITYFACTORY_H_
#define SRC_RTS_ENTITYFACTORY_H_

#include <map>
#include <string>
#include <glm/glm.hpp>
#include <json/json.h>
#include "Logger.h"

namespace rts {

class Entity;

class EntityFactory {
 public:
  static EntityFactory *get();

  Entity * construct(const std::string &cl,
                     const std::string &name, const Json::Value &params);

 private:
  EntityFactory();
  ~EntityFactory();

  LoggerPtr logger_;
};
};  // rts

#endif  // SRC_RTS_ENTITYFACTORY_H_
