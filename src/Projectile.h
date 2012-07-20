#include "Mobile.h"
#include "Logger.h"

class Projectile :
    public Mobile
{
public:
    Projectile();
    // TODO(zack): maybe it's actually better to pass
    // damage type/damage amount/effects as parameters here, so that the
    // weapon/actor just sends a dummy projectile.  Seems like we will have
    // duplicated params with this architecture
    Projectile(int64_t playerID, const glm::vec3 &pos, 
            const std::string &type, eid_t targetID);
    virtual ~Projectile() {}

    virtual const std::string getType() const { return "PROJECTILE"; }

    virtual void update(float dt);
    virtual bool needsRemoval() const;
    virtual void handleMessage(const Message &msg);

protected:
    eid_t targetID_;
    bool done_;

private:
    static LoggerPtr logger_;
};

