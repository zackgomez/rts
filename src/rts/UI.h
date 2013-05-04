#ifndef SRC_RTS_UI_H_
#define SRC_RTS_UI_H_
#include <functional>
#include <map>
#include <mutex>
#include <string>
#include <vector>
#include <glm/glm.hpp>
#include <SDL/SDL.h>
#include "common/Types.h"
#include "common/util.h"
#include "common/Types.h"
#include "rts/FontManager.h"
#include "rts/Graphics.h"

namespace rts {

class ModelEntity;
class UIWidget;
struct MapHighlight;

// Dispatches the event to the specified handler
// Vec2 params are [0, resolution] with 0,0 being the top left corner
void interpretSDLEvent(
    const SDL_Event &event,
    std::function<void(const glm::vec2 &, int)> mouseDownHandler,
    std::function<void(const glm::vec2 &, int)> mouseUpHandler,
    // parameter is bitmask of buttons
    std::function<void(const glm::vec2 &, int)> mouseMotionHandler,
    std::function<void(SDL_keysym)> keyPressHandler,
    std::function<void(SDL_keysym)> keyReleaseHandler,
    std::function<void()> quitEventHandler);

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
  bool handleKeyPress(SDL_keysym keysym);

  // Key capturing
  typedef std::function<bool(SDL_keysym keysym)> KeyCapturer;
  void setKeyCapturer(KeyCapturer kc) {
    capturer_ = kc;
    SDL_EnableUNICODE(SDL_ENABLE);
    SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY,
        SDL_DEFAULT_REPEAT_INTERVAL);
  }
  void clearKeyCapturer() {
    capturer_ = KeyCapturer();
    SDL_EnableUNICODE(SDL_DISABLE);
    SDL_EnableKeyRepeat(0, SDL_DEFAULT_REPEAT_INTERVAL);
  }

 private:
  std::map<std::string, UIWidget *> widgets_;
  KeyCapturer capturer_;
};
};  // rts
#endif  // SRC_RTS_UI_H_
