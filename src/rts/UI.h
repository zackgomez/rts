#ifndef SRC_RTS_UI_H_
#define SRC_RTS_UI_H_
#include <map>
#include <string>
#include <vector>
#include <glm/glm.hpp>
#include "common/util.h"
#include "common/Types.h"

namespace rts {

class RenderEntity;
class UIWidget;
struct MapHighlight;

class UI {
 public:
  UI();
  ~UI();

  void render(float dt);
  void renderEntity(const RenderEntity *e, const glm::mat4 &transform, float dt);
  void highlight(const glm::vec2 &mapCoord);
  void highlightEntity(id_t eid);

  void setChatActive(bool active) {
    chatActive_ = active;
  }
  void setChatBuffer(const std::string &buffer) {
    chatBuffer_ = buffer;
  }

 private:
  void renderChat();
  void renderMinimap();
  void renderHighlights(float dt);

  std::vector<MapHighlight> highlights_;
  std::map<id_t, float> entityHighlights_;

  std::vector<UIWidget *> widgets_;

  bool chatActive_;
  std::string chatBuffer_;
};

class UIWidget {
 public:
  virtual ~UIWidget() { }

  virtual void render(float dt) = 0;
};

class TextureWidget : public UIWidget {
 public:
  TextureWidget(
      const glm::vec2 &pos,
      const glm::vec2 &size,
      const std::string& texName);

  void render(float dt);

 private:
  glm::vec2 pos_;
  glm::vec2 size_;
  std::string texName_;
};

struct MapHighlight {
  glm::vec2 pos;
  float remaining;
};
};  // rts
#endif  // SRC_RTS_UI_H_
