#include "rts/ActionWidget.h"
#include <boost/algorithm/string.hpp>
#include <SDL/SDL.h>
#include "common/Collision.h"
#include "common/ParamReader.h"
#include "rts/FontManager.h"
#include "rts/UIAction.h"

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

  hoverTimer_ = hover_ ? hoverTimer_ + dt : 0.f;

  bool draw_tooltip = false;
  std::string tooltip;
  for (auto action : actions) {
    glm::vec4 color = bgcolor_;
    bool action_hover = hover_ && pointInBox(hoverPos_, center, size_, 0);
    if (action_hover) {
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

    if (action_hover && hoverTimer_ > fltParam("local.tooltipDelay")) {
      tooltip = action.getTooltip();
      draw_tooltip = true;
    }

    center.x += size_.x;
  }
  if (draw_tooltip) {
    float tooltip_font_height = fltParam("local.tooltipFontHeight");
    float tooltip_width = size_.x * 2.f;

    std::vector<std::string> tooltipLines;
    boost::split(tooltipLines, tooltip, boost::is_any_of("\n"));
    glm::vec2 pos = hoverPos_ -
      glm::vec2(0.f, (tooltipLines.size() + 1)* tooltip_font_height);

    glm::vec2 tooltip_rect_size = glm::vec2(
      tooltip_width,
      tooltip_font_height * tooltipLines.size() + tooltip_font_height / 2.f);

    drawRect(pos,
        tooltip_rect_size,
        glm::vec4(0.6f));

    for (auto& line : tooltipLines) {
      FontManager::get()->drawString(
          line,
          pos,
          tooltip_font_height);
      pos.y += tooltip_font_height;
    }
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
