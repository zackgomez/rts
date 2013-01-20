#pragma once
#include "rts/Widgets.h"
#include "common/Types.h"

namespace rts {

class MinimapWidget : public SizedWidget {
 public:
  MinimapWidget(const std::string &name, id_t localPlayerID);
  virtual ~MinimapWidget();

  void render(float dt);

 private:
  id_t localPlayerID_;
  std::string name_;

  glm::vec2 worldToMinimap(glm::vec2 world) const;
};

};  // rts
