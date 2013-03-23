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

UI* UI::instance_ = nullptr;


glm::vec2 UI::convertUIPos(const glm::vec2 &pos) {
  auto resolution = Renderer::get()->getResolution();
  return glm::vec2(
      pos.x < 0 ? pos.x + resolution.x : pos.x,
      pos.y < 0 ? pos.y + resolution.y : pos.y);
}

UI::UI() {
  invariant(!instance_, "Multiple UIs?");
  instance_ = this;
}

UI::~UI() {
  for (auto pair : widgets_) {
    delete pair.second;
  }
  instance_ = nullptr;
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
}

bool UI::handleMousePress(const glm::vec2 &screenCoord) {
  for (auto&& pair : widgets_) {
    if (pair.second->isClick(screenCoord)) {
      if (pair.second->handleClick(screenCoord)) {
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

  std::unique_lock<std::mutex> lock(funcMutex_);
  for (auto&& func : funcQueue_) {
    func();
  }
  funcQueue_.clear();
  lock.unlock();

  for (auto&& pair : widgets_) {
    pair.second->render(dt);
  }

  glEnable(GL_DEPTH_TEST);
}

void UI::renderEntity(const ModelEntity *e, float dt) {
  if (entityOverlayRenderer_) {
    entityOverlayRenderer_(e, dt);
  }
}

void UI::postToMainThread(const PostableFunction &func) {
  std::unique_lock<std::mutex> lock(funcMutex_);
  funcQueue_.push_back(func);
}
};  // rts
