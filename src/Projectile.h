#include "Mobile.h"
#include "Logger.h"

class Projectile :
    public Mobile
{
public:
    Projectile();
    Projectile(int64_t playerID, const glm::vec3 &pos, 
            const std::string &type, eid_t targetID);
    virtual ~Projectile() {}

    virtual void update(float dt);
    virtual bool needsRemoval() const;
    virtual void handleMessage(const Message &msg);

protected:
    eid_t targetID_;
    bool done_;

private:
    static LoggerPtr logger_;
};

