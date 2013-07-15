#include "rts/ActorPanelWidget.h"
#include <sstream>
#include <SDL/SDL.h>
#include "common/ParamReader.h"
#include "rts/Actor.h"
#include "rts/FontManager.h"
#include "rts/Game.h"
#include "rts/Renderer.h"

namespace rts {

ActorPanelWidget::ActorPanelWidget(
    const std::string &name,
    ActorFunc f)
  : name_(name),
    size_(uiSizeParam(name + ".dim")),
    center_(uiPosParam(name + ".center")),
    bgcolor_(vec4Param(name + ".bgcolor")),
    actorFunc_(f) {
}
 
ActorPanelWidget::~ActorPanelWidget() {
}

void ActorPanelWidget::render(float dt) {
  auto *actor = actorFunc_ ? actorFunc_() : nullptr;
  if (actor) {
    drawRectCenter(center_, size_, bgcolor_, 0.f);
    auto ui_info = actor->getUIInfo();

    const glm::vec2 part_size(
        size_.x * 0.25f,
        size_.y / 3.f);
    invariant(ui_info.parts.size() <= 6, "too many parts to render");
    for (int i = 0; i < ui_info.parts.size(); i++) {
      float x = i % 2 == 0
        ? center_.x - size_.x / 2.f + part_size.x / 2.f
        : center_.x + size_.x / 2.f - part_size.x / 2.f;
      float y = center_.y - size_.y / 2.f + part_size.y * (0.5f + i / 2);
      renderPart(glm::vec2(x, y), part_size, ui_info.parts[i]);
    }
  }
}

void ActorPanelWidget::renderPart(
    const glm::vec2 &center,
    const glm::vec2 &size,
    const Actor::UIPart &part) {
  drawRectCenter(center, size, glm::vec4(0,0,0,1), 0.f);
  glm::vec2 inside_size = size - 2.f;
  drawRectCenter(center, inside_size, glm::vec4(1,1,1,1), 0.f);

  float fact = part.health[0] / part.health[1];
  float health_height = inside_size.y / 5.f;
  float y = center.y + inside_size.y / 2.f - health_height / 2.f;
  drawRectCenter(
      glm::vec2(center.x, y),
      glm::vec2(inside_size.x, health_height),
      glm::vec4(0.8, 0.2, 0.2, 1.0),
      0.f);
  drawRectCenter(
      glm::vec2(center.x - inside_size.x / 2.f + inside_size.x * fact / 2.f, y),
      glm::vec2(inside_size.x * fact, health_height),
      glm::vec4(0.2, 0.9, 0.2, 1.0),
      0.f);
  std::stringstream ss;
  ss << glm::floor(part.health[0]) << " / " << glm::floor(part.health[1]);
  FontManager::get()->drawString(
      ss.str(),
      glm::vec2(center.x - inside_size.x / 2.f, y - health_height / 2.f),
      health_height);
}

void ActorPanelWidget::update(const glm::vec2 &pos, int buttons) {
  hover_ = pointInBox(pos, getCenter(), getSize(), 0.f);
  hoverPos_ = pos;

  if (!(buttons & SDL_BUTTON(1))) {
    press_ = false;
  } else if (hover_) {
    press_ = true;
  }
}

glm::vec2 ActorPanelWidget::getCenter() const {
  return center_;
}

glm::vec2 ActorPanelWidget::getSize() const {
  return size_;
}

};  // rts
