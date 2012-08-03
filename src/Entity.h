#pragma once
#include <cstdint>
#include <string>
#include <set>
#include <string>
#include <math.h>
#include <json/json.h>
#include "glm.h"
#include "Message.h"

namespace rts
{
const id_t NO_ENTITY = 0;
const id_t NO_PLAYER = 0;
const id_t GAME_ID   = 0;
const glm::vec3 NO_POSITION = glm::vec3(HUGE_VAL);

class Entity
{
public:
    const static id_t STARTING_ID;
    explicit Entity(const std::string &name, const Json::Value &params,
            bool mobile = false, bool targetable = false);
    virtual ~Entity();

    bool isTargetable() const { return targetable_; }
    bool isMobile() const { return mobile_; }

    // This entity's unique id
    id_t getID() const { return id_; }
    // The player than owns this entity, or NO_PLAYER
    id_t getPlayerID() const { return playerID_; }
    const std::string& getName() const { return name_; }
    virtual const std::string getType() const = 0;

    // This unit's position
    const glm::vec3 getPosition() const { return pos_; }
    // This unit's facing angle (relative to +x axis)
    const float getAngle() const { return angle_; }
    // This unit's rough bounding radius
    const float getRadius() const { return radius_; }

    // Interpolation functions
    virtual glm::vec3 getPosition(float dt) const;
    virtual float getAngle(float dt) const;

    virtual void handleMessage(const Message& msg) = 0;
    virtual void update(float dt) = 0;

protected:
    static const glm::vec3 getDirection(float angle);
    const glm::vec3 getDirection() const;
    float angleToTarget(const glm::vec3 &pos) const;

    float param(const std::string &p) const;
    std::string strParam(const std::string &p) const;
    bool hasParam(const std::string &p) const;
    bool hasStrParam(const std::string &p) const;

    glm::vec3 pos_;
    float angle_;
    float radius_;

    float speed_;
    float turnSpeed_;

private:
    static id_t lastID_;

    id_t id_;
    id_t playerID_;
    std::string name_;

    bool mobile_;
    bool targetable_;
};
}; // namespace rts
