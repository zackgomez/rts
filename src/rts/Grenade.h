#pragma once
#include "rts/GameEntity.h"

namespace rts {

class Grenade : public GameEntity {
 public:
  Grenade(id_t id, const std::string &name, const Json::Value &params);
  virtual ~Grenade() {}

  virtual void update(float dt);
  virtual void handleMessage(const Message &msg);

 private:
  glm::vec3 targetPos_;
  float explodeTimer_;
};

};  // rts
