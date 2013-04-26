#include "rts/ActionWidget.h"
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
    center_(uiPosParam(name + ".center")) {
  setOnClickListener(
      std::bind(&ActionWidget::handleRealClick, this, std::placeholders::_1));
}
 
ActionWidget::~ActionWidget() {
}

void ActionWidget::render(float dt) {
  auto actions = actionsFunc_();

  size_t num_actions = actions.size();
  // Center of the first box
  glm::vec2 center = center_ - glm::vec2(size_.x * 0.5f * (num_actions - 1), 0.f);

  for (auto action : actions) {
    // TODO(zack) draw something representative... with a tooltip or something
    drawRectCenter(center, size_, glm::vec4(0.3, 0.8, 0.1, 0.65));
    drawRectCenter(center, size_ - glm::vec2(5.f), glm::vec4(0, 0, 0, 1));

    center.x += size_.x;
  }
}

bool ActionWidget::isClick(const glm::vec2 &pos) const {
  auto num_actions = actionsFunc_().size();
  return pointInBox(pos, center_, glm::vec2(size_.x * num_actions, size_.y), 0.f);
}

bool ActionWidget::handleRealClick(const glm::vec2 &pos) const {
  auto actions = actionsFunc_();
  int idx = (pos.x - center_.x + actions.size() * size_.x) / size_.x;

  actionExecutor_(actions[idx]);
  return true;
}

};  // rts
