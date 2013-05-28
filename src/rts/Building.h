#ifndef SRC_RTS_BUILDING_H_
#define SRC_RTS_BUILDING_H_

#include "rts/Actor.h"

namespace rts {

	// TODO(zack): move this entirely into JS
class Building : public Actor {
 public:
  explicit Building(id_t id, const std::string &name, const Json::Value &params);
  virtual ~Building() { }

  virtual void collide(const GameEntity *, float dt) override { }

  virtual bool hasProperty(uint32_t property) const override {
    if (property == P_CAPPABLE) {
      return hasParam("captureTime");
    }
    return Actor::hasProperty(property);
  }

  // Returns if unit uid can capture this building
  bool canCapture(id_t uid) const;
};
};  // namespace rts

#endif  // SRC_RTS_BUILDING_H_
