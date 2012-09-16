#ifndef SRC_RTS_COLLISIONOBJECT_H_
#define SRC_RTS_COLLISIONOBJECT_H_

#include "rts/Entity.h"
#include "common/Logger.h"

namespace rts {

class CollisionObject : public Entity {
 public:
  explicit CollisionObject(const std::string &name, const Json::Value &params);
  virtual ~CollisionObject() { }

  static const std::string TYPE;
  virtual const std::string getType() const {
    return TYPE;
  }

  virtual void handleMessage(const Message &msg);
  virtual void update(float dt);
  virtual bool needsRemoval() const;

 private:
  static LoggerPtr logger_;
};
};  // namespace rts

#endif  // SRC_RTS_COLLISIONOBJECT_H_
