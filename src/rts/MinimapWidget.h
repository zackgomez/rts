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

  GLuint visibilityTex_;

  void renderBase(float dt);
  glm::vec2 worldToMinimap(const glm::vec3 &world) const;
};

};  // rts
