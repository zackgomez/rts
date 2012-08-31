#ifndef SRC_RTS_BUILDING_H_
#define SRC_RTS_BUILDING_H_

#include "Actor.h"
#include "Entity.h"
#include <glm/glm.hpp>
#include "Logger.h"

namespace rts {

class Building : public Actor {
 public:
  explicit Building(const std::string &name, const Json::Value &params);
  virtual ~Building() { }

  static const std::string TYPE;
  virtual const std::string getType() const {
    return TYPE;
  }

  virtual void handleMessage(const Message &msg);
  virtual void update(float dt);
  virtual bool needsRemoval() const;

  // Returns if unit uid can capture this building
  bool canCapture(id_t uid) const;
  // Returns if building is capturable at all
  bool isCappable() const {
    return hasParam("captureTime");
  }

  float getCap() const {
    return capAmount_;
  }
  float getMaxCap() const {
    return isCappable() ? param("captureTime") : 0.f;
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
  static LoggerPtr logger_;
  float capAmount_;
  id_t capperID_;
  id_t lastCappingPlayerID_;

  int capResetDelay_;

 protected:
  void enqueue(const Message &queue_order);
};
};  // namespace rts

#endif  // SRC_RTS_BUILDING_H_
