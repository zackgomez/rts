#pragma once
#include "Entity.h"
#include <glm/glm.hpp>

class Unit :
    public Entity
{
public:
    Unit(int64_t playerID);
    virtual ~Unit() { }

    virtual std::string getType() const { return "Unit"; }

    virtual void update(float dt);
    virtual bool needsRemoval() const;

protected:
    virtual void serialize(Json::Value &obj) const;
};

