#pragma once
#include "Actor.h"
#include "Entity.h"
#include <glm/glm.hpp>
#include "Logger.h"

class Building :
    public Actor
{
public:
    explicit Building(const std::string &name, const Json::Value &params);
    virtual ~Building() { }

    static const std::string TYPE;
    virtual const std::string getType() const { return TYPE; }

    virtual void handleMessage(const Message &msg);
    virtual void update(float dt);
    virtual bool needsRemoval() const;

private:
    static LoggerPtr logger_;

protected:
    void enqueue(const Message &queue_order);
};
