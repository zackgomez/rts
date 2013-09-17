#define GLM_SWIZZLE_XYZW
#include "rts/UI.h"
#include <sstream>
#include <functional>
#include "common/Clock.h"
#include "common/ParamReader.h"
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

void UI::update(const glm::vec2 &screen_coord, int buttons) {
  for (auto pair : widgets_) {
    pair.second->update(screen_coord, buttons);
  }
}

bool UI::handleMousePress(const glm::vec2 &screen_coord, int button) {
  for (auto&& pair : widgets_) {
    auto widget = pair.second;

    if (pointInBox(screen_coord, widget->getCenter(), widget->getSize(), 0.f)) {
      if (widget->handleClick(screen_coord, button)) {
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
};  // rts
