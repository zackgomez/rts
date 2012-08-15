#pragma once
#include <json/json.h>

namespace rts {

typedef Json::Value Message;

namespace MessageTypes {
const std::string ORDER          = "ORDER";
const std::string ATTACK         = "ATTACK";
const std::string SPAWN_ENTITY   = "SPAWN";
const std::string DESTROY_ENTITY = "DESTROY";
const std::string ADD_RESOURCE   = "RESOURCE";
};

};
