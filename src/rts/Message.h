#ifndef SRC_RTS_MESSAGE_H_
#define SRC_RTS_MESSAGE_H_

#include <json/json.h>

namespace rts {

typedef Json::Value Message;

namespace MessageTypes {
const std::string ORDER          = "ORDER";
// For taking damage
const std::string ATTACK         = "ATTACK";
const std::string CAPTURE        = "CAPTURE";
const std::string ADD_STAT       = "STAT";
};  // MessageTypes
};  // rts

#endif  // SRC_RTS_MESSAGE_H_
