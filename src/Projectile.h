#include "Entity.h"
#include "Logger.h"

class Projectile : public Entity
{
public:
    Projectile(const std::string &name, const Json::Value &params);
    virtual ~Projectile() {}

    static const std::string TYPE;
    virtual const std::string getType() const { return TYPE; }

    virtual void update(float dt);
    virtual bool needsRemoval() const;
    virtual void handleMessage(const Message &msg);

protected:
    eid_t targetID_;
    bool done_;

private:
    static LoggerPtr logger_;
};

