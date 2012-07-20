#pragma once
#include <cstdint>
#include <string>
#include <set>
#include <string>
#include <math.h>
#include <json/json.h>
#include "glm.h"
#include "Message.h"

typedef uint64_t eid_t;

const eid_t NO_ENTITY = 0;
const int64_t NO_PLAYER = -1;

class Entity
{
public:
    explicit Entity();
    explicit Entity(int64_t playerID, glm::vec3 pos);
    virtual ~Entity();

    // This entity's unique id
    eid_t getID() const { return id_; }
    // The player than owns this entity, or NO_PLAYER
    int64_t getPlayerID() const { return playerID_; }
    const std::string& getName() const { return name_; }
    virtual const std::string getType() const = 0;

    // This unit's position
    const glm::vec3 getPosition() const { return pos_; }
    // This unit's facing angle (relative to +x axis)
    const float getAngle() const { return angle_; }
    // This unit's rough bounding radius
    const float getRadius() const { return radius_; }
    // When true, will be destructed by engine
    virtual bool needsRemoval() const = 0;

    // Interpolation functions
    virtual glm::vec3 getPosition(float dt) const { return getPosition(); }
    virtual float getAngle(float dt) const { return getAngle(); }

    virtual void handleMessage(const Message& msg) = 0;
    virtual void update(float dt) = 0;

protected:
    eid_t id_;
    int64_t playerID_;
    std::string name_;

    glm::vec3 pos_;
    float angle_;
    float radius_;

private:
    static uint64_t lastID_;
};

