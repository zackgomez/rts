#define GLM_SWIZZLE_XYZW
#include "rts/UI.h"
#include <sstream>
#include <functional>
#include "common/Clock.h"
#include "common/ParamReader.h"
#include "rts/Actor.h"
#include "rts/Controller.h"
#include "rts/GameController.h"
#include "rts/Graphics.h"
#include "rts/FontManager.h"
#include "rts/Game.h"
#include "rts/Input.h"
#include "rts/Map.h"
#include "rts/Player.h"
#include "rts/Renderer.h"
#include "rts/ResourceManager.h"
#include "rts/Widgets.h"

namespace rts {

glm::vec2 UI::convertUIPos(const glm::vec2 &pos) {
  auto resolution = Renderer::get()->getResolution();
  return glm::vec2(
      pos.x < 0 ? pos.x + resolution.x : pos.x,
      pos.y < 0 ? pos.y + resolution.y : pos.y);
}

UI::UI() {
}

UI::~UI() {
  clearWidgets();
}

UIWidget* UI::getWidget(const std::string &name) {
  auto it = widgets_.find(name);
  if (it == widgets_.end()) {
    return nullptr;
  }
  return it->second;
}

void UI::addWidget(const std::string &name, UIWidget *widget) {
  widgets_[name] = widget;
  widget->setUI(this);
}

void UI::update(const glm::vec2 &screenCoord, int buttons) {
  for (auto pair : widgets_) {
    pair.second->update(screenCoord, buttons);
  }
}

bool UI::handleMousePress(const glm::vec2 &screenCoord, int button) {
  for (auto&& pair : widgets_) {
    auto widget = pair.second;

    if (pointInBox(screenCoord, widget->getCenter(), widget->getSize(), 0.f)) {
      if (widget->handleClick(screenCoord, button)) {
        return true;
      }
    }
  }

  return false;
}

bool UI::handleKeyPress(const KeyEvent &ev) {
  if (capturer_) {
    return capturer_(ev);
  }
  return false;
}

void UI::clearWidgets() {
  for (auto pair : widgets_) {
    delete pair.second;
  }
  widgets_.clear();
}

void UI::render(float dt) {
  glDisable(GL_DEPTH_TEST);

  for (const auto& pair : widgets_) {
    pair.second->render(dt);
  }

  for (const auto& f : deferedRenderers_) {
    f();
  }
  deferedRenderers_.clear();

  glEnable(GL_DEPTH_TEST);
}

int convertSDLMouseButton(int button) {
  int ret = 0;
  if (button & SDL_BUTTON(1)) {
    ret |= MouseButton::LEFT;
  }
  if (button & SDL_BUTTON(2)) {
    ret |= MouseButton::RIGHT;
  }

  if (button == SDL_BUTTON_LEFT) {
    ret = MouseButton::LEFT;
  }
  if (button == SDL_BUTTON_RIGHT) {
    ret = MouseButton::RIGHT;
  }
  if (button == SDL_BUTTON_WHEELUP) {
    ret = MouseButton::WHEEL_UP;
  }
  if (button == SDL_BUTTON_WHEELDOWN) {
    ret = MouseButton::WHEEL_DOWN;
  }

  return ret;
}

KeyEvent convertSDLKeyEvent(SDL_KeyboardEvent sdl_event) {
  KeyEvent ret;
  ret.action = sdl_event.type == SDL_KEYDOWN
    ? InputAction::INPUT_PRESS
    : InputAction::INPUT_RELEASE;
  // TODO(zack): fill this out
  ret.mods = 0;

  ret.key = 0;
  switch (sdl_event.keysym.sym) {
  case SDLK_ESCAPE:
    ret.key = INPUT_KEY_ESC;
    break;
  case SDLK_RETURN:
    ret.key = INPUT_KEY_RETURN;
    break;
  case SDLK_BACKSPACE:
    ret.key = INPUT_KEY_BACKSPACE;
    break;
  case SDLK_LSHIFT:
    ret.key = INPUT_KEY_LEFT_SHIFT;
    break;
  case SDLK_RSHIFT:
    ret.key = INPUT_KEY_RIGHT_SHIFT;
    break;
  case SDLK_LCTRL:
    ret.key = INPUT_KEY_LEFT_CTRL;
    break;
  case SDLK_RCTRL:
    ret.key = INPUT_KEY_RIGHT_CTRL;
    break;
  case SDLK_LALT:
    ret.key = INPUT_KEY_LEFT_ALT;
    break;
  case SDLK_RALT:
    ret.key = INPUT_KEY_RIGHT_ALT;
    break;
  case SDLK_DOWN:
    ret.key = INPUT_KEY_DOWN;
    break;
  case SDLK_UP:
    ret.key = INPUT_KEY_UP;
    break;
  case SDLK_LEFT:
    ret.key = INPUT_KEY_LEFT;
    break;
  case SDLK_RIGHT:
    ret.key = INPUT_KEY_RIGHT;
    break;
  case SDLK_F10:
    ret.key = INPUT_KEY_F10;
    break;
  default:
    int sym = sdl_event.keysym.sym;
    if (isalpha(sym)) {
      ret.key = toupper(sdl_event.keysym.sym);
    } else if (isprint(sdl_event.keysym.sym)) {
      ret.key = sdl_event.keysym.sym;
    }
  }

  return ret;
}

void interpretSDLEvent(
    const SDL_Event &event,
    std::function<void(const glm::vec2 &, int)> mouseDownHandler,
    std::function<void(const glm::vec2 &, int)> mouseUpHandler,
    std::function<void(const glm::vec2 &, int)> mouseMotionHandler,
    std::function<void(const KeyEvent &)> keyPressHandler,
    std::function<void(const KeyEvent &)> keyReleaseHandler,
    std::function<void()> quitEventHandler) {
  glm::vec2 screenCoord;

  switch (event.type) {
  case SDL_KEYDOWN:
    keyPressHandler(convertSDLKeyEvent(event.key));
    break;
  case SDL_KEYUP:
    keyReleaseHandler(convertSDLKeyEvent(event.key));
    break;
  case SDL_MOUSEBUTTONDOWN:
    screenCoord = glm::vec2(event.button.x, event.button.y);
    mouseDownHandler(screenCoord, convertSDLMouseButton(event.button.button));
    break;
  case SDL_MOUSEBUTTONUP:
    screenCoord = glm::vec2(event.button.x, event.button.y);
    mouseUpHandler(screenCoord, convertSDLMouseButton(event.button.button));
    break;
  case SDL_MOUSEMOTION:
    screenCoord = glm::vec2(event.motion.x, event.motion.y);
    mouseMotionHandler(screenCoord, convertSDLMouseButton(event.motion.state));
    break;
  case SDL_QUIT:
    quitEventHandler();
    break;
  }
}
};  // rts
