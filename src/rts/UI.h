#ifndef SRC_RTS_UI_H
#define SRC_RTS_UI_H

namespace rts {

class UI {
 public:
  UI();
  ~UI();

  void render(float dt);

 private:
  void renderMinimap();
};

};  // rts
#endif  // SRC_RTS_UI_H
