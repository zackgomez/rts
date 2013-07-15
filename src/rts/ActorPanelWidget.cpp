#include "rts/ActorPanelWidget.h"
#include <boost/algorithm/string.hpp>
#include <SDL/SDL.h>
#include "common/Collision.h"
#include "common/ParamReader.h"
#include "rts/FontManager.h"
#include "rts/Game.h"
#include "rts/Renderer.h"
#include "rts/UIAction.h"

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

    const glm::vec2 part_size(
        size_.x * 0.25f,
        size_.y / 3.f);
    for (int i = 0; i < 6; i++) {
      float x = i % 2
        ? center_.x - size_.x / 2.f
        : center_.x + size_.x / 2.f - part_size.x / 2.f;
      float y = center_.y - size_.y / 2.f + part_size.y * (0.5f + i / 2);
      drawRectCenter(glm::vec2(x, y), part_size, glm::vec4(1,1,1,1), 0.f);
    }
  }
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
