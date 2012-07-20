#include "Building.h"
#include "ParamReader.h"
#include "MessageHub.h"
#include <string>
#include <iostream>

LoggerPtr Building::logger_;

Building::Building(int64_t playerID, const glm::vec3 &pos, const std::string &name) :
    Entity(playerID, pos),
    Actor(name)
{
    pos_ = pos;
    name_ = name;
    radius_ = 1.0f;
}

void Building::handleMessage(const Message &msg)
{
    if (msg["type"] == MessageTypes::ENQUEUE)
    {
        enqueue(msg);
    }
    else if (msg["type"] == MessageTypes::ATTACK)
    {
        assert(msg.isMember("pid"));
        assert(msg.isMember("damage"));

        // TODO(zack) figure out how to deal with this case
        assert(msg["pid"].asInt64() != playerID_);
        // Just take damage for now
        health_ -= msg["damage"].asFloat();
    }
    else
    {
    }
}

void Building::enqueue(const Message &queue_order)
{
    assert(queue_order["type"] == MessageTypes::ENQUEUE);
    assert(queue_order.isMember("prod"));
    std::string prod_name = queue_order["prod"].asString();
    Production prod;
    prod.time = getParam(prod_name + ".buildTime");
    prod.max_time = getParam(prod_name + ".buildTime");
    prod.name = prod_name;
    Message spawnMsg;
    spawnMsg["type"] = MessageTypes::SPAWN_ENTITY;
    spawnMsg["to"] = (Json::Value::UInt64) NO_ENTITY;
    spawnMsg["entity_type"] = "UNIT";
    spawnMsg["entity_pid"] = (Json::Value::Int64) playerID_;
    spawnMsg["entity_pos"] = toJson(pos_ + glm::vec3(radius_ * cos(angle_), 0.f, radius_ * sin(angle_)));
    spawnMsg["unit_name"] = prod_name;
    prod.msg = spawnMsg;
    production_queue_.push(prod);
}

void Building::update(float dt)
{
    if (production_queue_.empty()) return;
    production_queue_.front().time -= dt;
    if (production_queue_.front().time <= 0)
    {
        MessageHub::get()->sendMessage(production_queue_.front().msg);
        production_queue_.pop();
    }
}

bool Building::needsRemoval() const
{
    return health_ <= 0.f;
}
