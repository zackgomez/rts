#ifndef SRC_RTS_UI_H_
#define SRC_RTS_UI_H_
#include <string>
#include <vector>
#include "common/util.h"

namespace rts {

class UIWidget;

class UI {
 public:
  UI();
  ~UI();

  void render(float dt);

  void setChatActive(bool active) {
    chatActive_ = active;
  }
  void setChatBuffer(const std::string &buffer) {
    chatBuffer_ = buffer;
  }

 private:
  void renderChat();
  void renderMinimap();

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
};  // rts
#endif  // SRC_RTS_UI_H_
