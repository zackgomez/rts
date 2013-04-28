#pragma once
#include "rts/Widgets.h"
#include "common/Types.h"

namespace rts {

class MinimapWidget final : public StaticWidget {
 public:
  MinimapWidget(const std::string &name, id_t localPlayerID);
  virtual ~MinimapWidget();

  void render(float dt);
  virtual bool handleClick(const glm::vec2 &pos) override;

  // called with position [0,1] in minimap coordinates
  typedef std::function<void(const glm::vec2 &)> MinimapListener;
  void setMinimapListener(MinimapListener l) {
    listener_ = l;
  }

 private:
  id_t localPlayerID_;
  std::string name_;
  MinimapListener listener_;

  GLuint visibilityTex_;

  void renderBase(float dt);
  glm::vec2 worldToMinimap(const glm::vec3 &world) const;
};

};  // rts
