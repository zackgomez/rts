#ifndef SRC_RTS_COLLISIONOBJECT_H_
#define SRC_RTS_COLLISIONOBJECT_H_

#include "rts/GameEntity.h"
#include "common/Logger.h"

namespace rts {

class CollisionObject : public GameEntity {
 public:
  explicit CollisionObject(id_t id, const std::string &name, const Json::Value &params);
  virtual ~CollisionObject() { }

  virtual void handleMessage(const Message &msg);
  virtual void update(float dt);
  virtual bool needsRemoval() const;
};
};  // namespace rts

#endif  // SRC_RTS_COLLISIONOBJECT_H_
