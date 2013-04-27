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
    center_(uiPosParam(name + ".center")),
    bgcolor_(vec4Param(name + ".bgcolor")) {
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
    drawRectCenter(center, size_, bgcolor_);
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

bool ActionWidget::isClick(const glm::vec2 &pos) const {
  auto num_actions = actionsFunc_().size();
  return pointInBox(pos, center_, glm::vec2(size_.x * num_actions, size_.y), 0.f);
}

bool ActionWidget::handleRealClick(const glm::vec2 &pos) const {
  auto actions = actionsFunc_();
  int idx = 1 / size_.x * (pos.x - center_.x + actions.size() * size_.x / 2);

  actionExecutor_(actions[idx]);
  return true;
}

};  // rts
