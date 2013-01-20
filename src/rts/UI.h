#ifndef SRC_RTS_UI_H_
#define SRC_RTS_UI_H_
#include <map>
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

class UI {
 public:
  static glm::vec2 convertUIPos(const glm::vec2 &pos);

  UI();
  ~UI();

  static UI* get() {
    if (!instance_) {
      instance_ = new UI;
    }
    return instance_;
  }

  void addWidget(const std::string &name, UIWidget *widget);
  UIWidget *getWidget(const std::string &name);
  void clearWidgets();

  bool handleMousePress(const glm::vec2 &loc);
  bool handleKeyPress(SDL_keysym keysym);

  void render(float dt);
  void renderEntity(const ModelEntity *e, const glm::mat4 &transform, float dt);

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
  static UI* instance_;

  std::map<std::string, UIWidget *> widgets_;
  KeyCapturer capturer_;
};
};  // rts
#endif  // SRC_RTS_UI_H_
