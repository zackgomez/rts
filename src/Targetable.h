#pragma once

class Targetable
{
public:
    explicit Targetable();
    explicit Targetable(float health);
    virtual ~Targetable();
    float getHealth() const { return health_; }
    float getMaxHealth() const { return health_max_; }
protected:
    float health_max_;
    float health_;
};

