#ifndef SRC_RTS_PROJECTILE_H_
#define SRC_RTS_PROJECTILE_H_

#include "rts/GameEntity.h"
#include "common/Logger.h"

namespace rts {

class Projectile : public GameEntity {
 public:
  Projectile(id_t id, const std::string &name, const Json::Value &params);
  virtual ~Projectile() {}

  virtual void update(float dt);
  virtual void handleMessage(const Message &msg);

 protected:
  id_t targetID_;
  id_t ownerID_;
};
};  // rts

#endif  // SRC_RTS_PROJECTILE_H_
