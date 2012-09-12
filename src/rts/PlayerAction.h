#ifndef SRC_RTS_PLAYERACTION_H_
#define SRC_RTS_PLAYERACTION_H_

#include <json/json.h>
#include <string>

typedef Json::Value PlayerAction;

namespace ActionTypes {
const std::string DONE = "DONE";
const std::string MOVE = "MOVE";
const std::string ATTACK = "ATTACK";
const std::string CAPTURE = "CAPTURE";
const std::string STOP = "STOP";
const std::string ENQUEUE = "ENQUEUE";
const std::string CHAT = "CHAT";

const std::string LEAVE_GAME = "LEAVE_GAME";
};

#endif  // SRC_RTS_PLAYERACTION_H_