#ifndef SRC_RTS_UI_H_
#define SRC_RTS_UI_H_
#include <string>

namespace rts {

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

  bool chatActive_;
  std::string chatBuffer_;
};
};  // rts
#endif  // SRC_RTS_UI_H_
