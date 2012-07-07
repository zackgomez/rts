#pragma once
#include <cstdint>
#include <string>
#include <set>
#include <string>
#include <math.h>
#include <json/json.h>
#include <glm/glm.hpp>

typedef uint64_t eid_t;

const eid_t NO_ENTITY = 0;
const int64_t NO_PLAYER = -1;

class EntityListener;

class Entity
{
public:
    explicit Entity();
    explicit Entity(int64_t playerID);
    virtual ~Entity();

    virtual std::string getType() const = 0;
    eid_t getID() const { return id_; }
    int64_t getPlayerID() const { return playerID_; }

    const glm::vec3 getPosition() const { return pos_; }
    const glm::vec2 getDirection() const { return glm::vec2(cos(angle_*M_PI/180), sin(angle_*M_PI/180)); }
    // This unit's rough bounding radius
    const float getRadius() const { return radius_; }

    virtual void registerListener(EntityListener *listener);
    virtual void removeListener(EntityListener *listener);

    virtual void update(float dt) = 0;
    // When true, will be destructed by engine
    virtual bool needsRemoval() const = 0;

    std::string serialize() const;

protected:
    // TODO a serialize protected helper function
    virtual void serialize(Json::Value &obj) const = 0;

    glm::vec3 pos_;
    glm::vec2 dir_;
    float angle_;
    float radius_;

private:
    eid_t id_;
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
    // When the object is "killed" or destroyed in game
    virtual void killed() { }
};

