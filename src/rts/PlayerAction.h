#ifndef SRC_RTS_PLAYERACTION_H_
#define SRC_RTS_PLAYERACTION_H_

#include <json/json.h>
#include <string>

typedef Json::Value PlayerAction;

namespace ActionTypes {
const std::string ORDER = "ORDER";
const std::string DONE = "DONE";
const std::string CHAT = "CHAT";
const std::string LEAVE_GAME = "LEAVE_GAME";
};

namespace OrderTypes {
const std::string MOVE = "MOVE";
const std::string ATTACK = "ATTACK";
const std::string CAPTURE = "CAPTURE";
const std::string STOP = "STOP";
const std::string ACTION = "ACTION";
}

#endif  // SRC_RTS_PLAYERACTION_H_
