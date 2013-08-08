#ifndef SRC_RTS_UI_H_
#define SRC_RTS_UI_H_
#include <functional>
#include <map>
#include <mutex>
#include <string>
#include <vector>
#include <glm/glm.hpp>
#include "common/Types.h"
#include "common/util.h"
#include "common/Types.h"
#include "rts/FontManager.h"
#include "rts/Graphics.h"
#include "rts/Input.h"

namespace rts {

class ModelEntity;
class UIWidget;
struct MapHighlight;

class UI {
 public:
  static glm::vec2 convertUIPos(const glm::vec2 &pos);

  UI();
  ~UI();

  void render(float dt);

  // Widget functions
  void addWidget(const std::string &name, UIWidget *widget);
  UIWidget *getWidget(const std::string &name);
  void clearWidgets();

  // Input handling
  void update(const glm::vec2 &loc, int buttons);
  bool handleMousePress(const glm::vec2 &loc, int button);
  bool handleKeyPress(const KeyEvent &ev);

  // Key capturing
  typedef std::function<bool(const KeyEvent &ev)> KeyCapturer;
  void setKeyCapturer(KeyCapturer kc) {
    capturer_ = kc;
  }
  void clearKeyCapturer() {
    capturer_ = KeyCapturer();
  }

  typedef std::function<void()> DeferedRenderFunc;
  void addDeferedRenderFunc(DeferedRenderFunc f) {
    deferedRenderers_.push_back(f);
  }

 private:
  std::map<std::string, UIWidget *> widgets_;
  std::vector<DeferedRenderFunc> deferedRenderers_;
  KeyCapturer capturer_;
};
};  // rts
#endif  // SRC_RTS_UI_H_
