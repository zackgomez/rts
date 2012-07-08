#include "Entity.h"

uint64_t Entity::lastID_ = 1;

Entity::Entity()
: playerID_(NO_PLAYER)
, id_(lastID_++)
{
}

Entity::Entity(int64_t playerID)
: playerID_(playerID)
, id_(lastID_++)
{
}

Entity::~Entity()
{
}

std::string Entity::serialize() const
{
	Json::Value obj;
	obj["type"] = getType();
	obj["id"] = (Json::Value::UInt64) id_;
	obj["pid"] = (Json::Value::Int64) playerID_;
    obj["pos"] = toJson(pos_);
    obj["angle"] = angle_;
    obj["r"] = radius_;

	serialize(obj);

	Json::FastWriter writer;
	return writer.write(obj);
}

