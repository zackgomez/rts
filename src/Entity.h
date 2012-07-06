#pragma once
#include <cstdint>
#include <string>
#include <set>
#include <string>
#include <json/json.h>

const int64_t NO_PLAYER = -1;

class EntityListener;

class Entity
{
public:
    explicit Entity();
    explicit Entity(int64_t playerID);
    virtual ~Entity();

    virtual std::string getType() = 0;
    uint64_t getID() const { return id_; }
    int64_t getPlayerID() const { return playerID_; }

    virtual void registerListener(EntityListener *listener);
    virtual void removeListener(EntityListener *listener);

    virtual void update(float dt) = 0;
    // When true, will be destructed by engine
    virtual bool needsRemoval() const = 0;

    std::string serialize() const;

protected:
    // TODO a serialize protected helper function
    virtual void serialize(Json::Value &obj) = 0;

private:
    uint64_t id_;
    int64_t playerID_;
    std::set<EntityListener *> listeners_;

    static uint64_t lastID_;
};


class EntityListener
{
public:
    virtual ~EntityListener() { }

    // Called when the entity is being removed (from the destructor)
    // You should not query the object during or after this point
    virtual void removal() = 0;
};

