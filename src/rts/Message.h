#ifndef SRC_RTS_MESSAGE_H_
#define SRC_RTS_MESSAGE_H_

#include <json/json.h>

namespace rts {

typedef Json::Value Message;

namespace MessageTypes {
const std::string ORDER          = "ORDER";
const std::string ATTACK         = "ATTACK";
const std::string CAPTURE        = "CAPTURE";
const std::string SPAWN_ENTITY   = "SPAWN";
const std::string DESTROY_ENTITY = "DESTROY";
const std::string ADD_RESOURCE   = "RESOURCE";
const std::string ADD_VP         = "VP";
};  // MessageTypes
};  // rts

#endif  // SRC_RTS_MESSAGE_H_
