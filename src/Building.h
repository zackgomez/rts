#pragma once
#include "Actor.h"
#include "Entity.h"
#include <glm/glm.hpp>
#include "Logger.h"

class Building :
    public Entity, public Actor
{
public:
    explicit Building(int64_t playerID, const glm::vec3 &pos,
            const std::string &name);
    virtual ~Building() { }

    virtual const std::string getType() const { return "BUILDING"; }

    virtual void handleMessage(const Message &msg);
    virtual void update(float dt);
    virtual bool needsRemoval() const;

private:
    static LoggerPtr logger_;

protected:
    void enqueue(const Message &queue_order);
};
