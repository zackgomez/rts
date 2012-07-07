#pragma once
#include <json/json.h>
#include <string>

typedef Json::Value PlayerAction;

namespace ActionTypes
{
const std::string NONE = "NONE";
const std::string MOVE = "MOVE";
};
