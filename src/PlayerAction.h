#pragma once
#include <json/json.h>
#include <string>

typedef Json::Value PlayerAction;

namespace ActionTypes
{
const std::string DONE = "DONE";
const std::string MOVE = "MOVE";
const std::string ATTACK = "ATTACK";
const std::string STOP = "STOP";

const std::string LEAVE_GAME = "LEAVE_GAME";
};
