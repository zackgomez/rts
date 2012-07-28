#include <string>
#include <iostream>
#include "Building.h"
#include "ParamReader.h"
#include "MessageHub.h"
#include "Unit.h"

LoggerPtr Building::logger_;
const std::string Building::TYPE = "BUILDING";

Building::Building(const std::string &name, const Json::Value &params) :
    Actor(name, params)
{
}

void Building::handleMessage(const Message &msg)
{
    Actor::handleMessage(msg);
}

void Building::update(float dt)
{
    Actor::update(dt);
}

bool Building::needsRemoval() const
{
    return health_ <= 0.f;
}
