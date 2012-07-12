#pragma once

class Targetable
{
public:
    explicit Targetable();
    explicit Targetable(float health);
    virtual ~Targetable();
    const float getHealth() const { return health_current_; }
    const float getMaxHealth() const { return health_max_; }
protected:
    float health_max_;
    float health_current_;
};

