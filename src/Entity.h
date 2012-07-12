#pragma once
#include <cstdint>
#include <string>
#include <set>
#include <string>
#include <math.h>
#include <json/json.h>
#include "glm.h"
#include "MessageHub.h"

typedef uint64_t eid_t;

const eid_t NO_ENTITY = 0;
const int64_t NO_PLAYER = -1;

class EntityListener;

class Entity
{
public:
    explicit Entity();
    explicit Entity(int64_t playerID, glm::vec3 pos);
    virtual ~Entity();

    virtual std::string getType() const = 0;
    eid_t getID() const { return id_; }
    int64_t getPlayerID() const { return playerID_; }

    const glm::vec3 getPosition() const { return pos_; }
    const float getAngle() const {return angle_;}
    // This unit's rough bounding radius
    const float getRadius() const { return radius_; }

    virtual void handleMessage(const Message& msg) = 0;

    virtual void update(float dt) = 0;
    // When true, will be destructed by engine
    virtual bool needsRemoval() const = 0;

protected:
    eid_t id_;
    int64_t playerID_;

    glm::vec3 pos_;
    float angle_;
    float radius_;

private:
    static uint64_t lastID_;
};

