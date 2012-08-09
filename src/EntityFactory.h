#pragma once
#include <map>
#include <string>
#include "glm.h"
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

};
