#define GLM_SWIZZLE_XYZW
#include "rts/UI.h"
#include <sstream>
#include <functional>
#include "common/Clock.h"
#include "common/ParamReader.h"
#include "rts/Actor.h"
#include "rts/Building.h"
#include "rts/Controller.h"
#include "rts/GameController.h"
#include "rts/Graphics.h"
#include "rts/FontManager.h"
#include "rts/Game.h"
#include "rts/Map.h"
#include "rts/Player.h"
#include "rts/Renderer.h"
#include "rts/ResourceManager.h"
#include "rts/Unit.h"
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

bool UI::handleKeyPress(SDL_keysym keysym) {
  if (capturer_) {
    return capturer_(keysym);
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

  for (auto&& pair : widgets_) {
    pair.second->render(dt);
  }

  glEnable(GL_DEPTH_TEST);
}

void interpretSDLEvent(
    const SDL_Event &event,
    std::function<void(const glm::vec2 &, int)> mouseDownHandler,
    std::function<void(const glm::vec2 &, int)> mouseUpHandler,
    std::function<void(const glm::vec2 &, int)> mouseMotionHandler,
    std::function<void(SDL_keysym)> keyPressHandler,
    std::function<void(SDL_keysym)> keyReleaseHandler,
    std::function<void()> quitEventHandler) {
  glm::vec2 screenCoord;

  switch (event.type) {
  case SDL_KEYDOWN:
    keyPressHandler(event.key.keysym);
    break;
  case SDL_KEYUP:
    keyReleaseHandler(event.key.keysym);
    break;
  case SDL_MOUSEBUTTONDOWN:
    screenCoord = glm::vec2(event.button.x, event.button.y);
    mouseDownHandler(screenCoord, event.button.button);
    break;
  case SDL_MOUSEBUTTONUP:
    screenCoord = glm::vec2(event.button.x, event.button.y);
    mouseUpHandler(screenCoord, event.button.button);
    break;
  case SDL_MOUSEMOTION:
    screenCoord = glm::vec2(event.motion.x, event.motion.y);
    mouseMotionHandler(screenCoord, event.motion.state);
    break;
  case SDL_QUIT:
    quitEventHandler();
    break;
  }
}
};  // rts
