#pragma once
#include "rts/Widgets.h"
#include "common/Types.h"

namespace rts {

class MinimapWidget final : public StaticWidget {
 public:
  MinimapWidget(const std::string &name, id_t localPlayerID);
  virtual ~MinimapWidget();

  virtual bool handleClick(const glm::vec2 &pos, int button) override;
  void render(float dt);

  // called with position [0,1] in minimap coordinates
  typedef std::function<void(const glm::vec2 &, int button)> MinimapListener;
  void setMinimapListener(MinimapListener l) {
    listener_ = l;
  }

 private:
  id_t localPlayerID_;
  std::string name_;
  MinimapListener listener_;

  GLuint visibilityTex_;

  void renderBase();
  glm::vec2 worldToMinimap(const glm::vec3 &world) const;
};

};  // rts
