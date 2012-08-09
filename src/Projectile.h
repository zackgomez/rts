#include "Entity.h"
#include "Logger.h"

namespace rts {

class Projectile : public Entity {
public:
  Projectile(const std::string &name, const Json::Value &params);
  virtual ~Projectile() {}

  static const std::string TYPE;
  virtual const std::string getType() const {
    return TYPE;
  }

  virtual void update(float dt);
  virtual void handleMessage(const Message &msg);

protected:
  id_t targetID_;
  id_t ownerID_;

private:
  static LoggerPtr logger_;
};

};
