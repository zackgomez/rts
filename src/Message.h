#pragma once
#include <json/json.h>

typedef Json::Value Message;

namespace MessageTypes
{
    const std::string ORDER = "ORDER";
    const std::string SPAWN_ENTITY = "SPAWN";
};


