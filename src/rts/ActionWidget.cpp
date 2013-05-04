#include "rts/ActionWidget.h"
#include <SDL/SDL.h>
#include "common/Collision.h"
#include "common/ParamReader.h"

namespace rts {

ActionWidget::ActionWidget(
    const std::string &name,
    ActionFunc actions,
    ActionExecutor executor) 
  : actionsFunc_(actions),
    actionExecutor_(executor),
    size_(uiSizeParam(name + ".size")),
    center_(uiPosParam(name + ".center")),
    bgcolor_(vec4Param(name + ".bgcolor")) {
}
 
ActionWidget::~ActionWidget() {
}

void ActionWidget::render(float dt) {
  auto actions = actionsFunc_();

  size_t num_actions = actions.size();
  // Center of the first box
  glm::vec2 center = center_ - glm::vec2(size_.x * 0.5f * (num_actions - 1), 0.f);

  for (auto action : actions) {
    glm::vec4 color = bgcolor_;
    if (hover_ && pointInBox(hoverPos_, center, size_, 0)) {
      float fact = press_ ? 0.8f : 1.2f;
      color *= glm::vec4(fact, fact, fact, 1.f);
    }
    drawRectCenter(center, size_, color);
    invariant(
        action.actor_action.isMember("texture"),
        "missing texture for action");
    drawTextureCenter(
        center,
        size_ - glm::vec2(5.f),
        ResourceManager::get()->getTexture(action.actor_action["texture"].asString()),
        glm::vec4(0, 0, 1, 1));

    center.x += size_.x;
  }
}

void ActionWidget::update(const glm::vec2 &pos, int buttons) {
  hover_ = pointInBox(pos, getCenter(), getSize(), 0.f);
  hoverPos_ = pos;

  if (!(buttons & SDL_BUTTON(1))) {
    if (press_ & hover_) {
      auto actions = actionsFunc_();
      int idx = 1 / size_.x * (pos.x - center_.x + actions.size() * size_.x / 2);
      actionExecutor_(actions[idx]);
    }
    press_ = false;
  } else if (hover_) {
    press_ = true;
  }
}

glm::vec2 ActionWidget::getCenter() const {
  return center_;
}

glm::vec2 ActionWidget::getSize() const {
  return glm::vec2(size_.x * actionsFunc_().size(), size_.y);
}

};  // rts
