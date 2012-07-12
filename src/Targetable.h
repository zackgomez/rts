#pragma once

class Targetable
{
public:
    explicit Targetable();
    explicit Targetable(float start_health);
    virtual ~Targetable();
    const float getHealth() const { return health; }
    void setHealth(float new_health) { health = new_health; }
    // damage function makes it easier to subtract health
    void damage(float damage) { setHealth(health - damage); }
protected:
    float health;
};
