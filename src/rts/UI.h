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

  typedef std::function<void(const ModelEntity *, const glm::mat4&, float)>
      EntityOverlayRenderer;
  void setEntityOverlayRenderer(EntityOverlayRenderer r) {
    entityOverlayRenderer_ = r;
  }

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

  typedef std::function<void()> PostableFunction;
  void postToMainThread(const PostableFunction& func);

 private:
  static UI* instance_;

  EntityOverlayRenderer entityOverlayRenderer_;
  std::map<std::string, UIWidget *> widgets_;
  std::vector<PostableFunction> funcQueue_;
  std::mutex funcMutex_;
  KeyCapturer capturer_;
};
};  // rts
#endif  // SRC_RTS_UI_H_
