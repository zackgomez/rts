#ifndef SRC_RTS_COLLISIONOBJECT_H_
#define SRC_RTS_COLLISIONOBJECT_H_

#include "rts/GameEntity.h"
#include "common/Logger.h"

namespace rts {

// TODO(zack): fucking kill this thing.  it should be just a model entity
class CollisionObject : public GameEntity {
 public:
  explicit CollisionObject(id_t id, const std::string &name, const Json::Value &params);
  virtual ~CollisionObject() { }
};
};  // namespace rts

#endif  // SRC_RTS_COLLISIONOBJECT_H_
