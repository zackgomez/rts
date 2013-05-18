#ifndef SRC_RTS_BUILDING_H_
#define SRC_RTS_BUILDING_H_

#include "rts/Actor.h"

namespace rts {

class Building : public Actor {
 public:
  explicit Building(id_t id, const std::string &name, const Json::Value &params);
  virtual ~Building() { }

  virtual void handleMessage(const Message &msg) override;
  virtual void update(float dt) override;

  virtual bool hasProperty(uint32_t property) const override {
    if (property == P_CAPPABLE) {
      return hasParam("captureTime");
    }
    return Actor::hasProperty(property);
  }

  // Returns if unit uid can capture this building
  bool canCapture(id_t uid) const;

  float getCap() const {
    return capAmount_;
  }
  float getMaxCap() const {
    return hasProperty(P_CAPPABLE) ? fltParam("captureTime") : 0.f;
  }
  id_t getCapperID() const {
    return capperID_;
  }
  void setCapperID(id_t id) {
    capperID_ = id;
  }
  id_t getLastCappingPlayerID() const {
    return lastCappingPlayerID_;
  }

 private:
  float capAmount_;
  id_t capperID_;
  id_t lastCappingPlayerID_;

  int capResetDelay_;

 protected:
  void enqueue(const Message &queue_order);
};
};  // namespace rts

#endif  // SRC_RTS_BUILDING_H_
