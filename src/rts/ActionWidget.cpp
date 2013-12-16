#include "rts/ActionWidget.h"
#include <boost/algorithm/string.hpp>
#include "common/Collision.h"
#include "common/ParamReader.h"
#include "rts/FontManager.h"
#include "rts/Game.h"
#include "rts/Input.h"
#include "rts/Renderer.h"
#include "rts/UIAction.h"

namespace rts {

ActionWidget::ActionWidget(
    const std::string &name,
    ActionFunc actions,
    ActionExecutor executor) 
  : actionsFunc_(actions),
    actionExecutor_(executor),
    bgcolor_(vec4Param(name + ".bgcolor")) {
  center_ = uiPosParam(name + ".center");
  actionSize_ = uiSizeParam(name + ".size");
  size_ = glm::vec2(0.f);
}
 
ActionWidget::~ActionWidget() {
}

void ActionWidget::render(float dt) {
  const auto t = Renderer::get()->getGameTime();
  auto actions = actionsFunc_();

  size_t num_actions = actions.size();
  // Center of the first box
  glm::vec2 center = center_ - glm::vec2(actionSize_.x * 0.5f * (num_actions - 1), 0.f);

  hoverTimer_ = hover_ ? hoverTimer_ + dt : 0.f;

  bool draw_tooltip = false;
  std::string tooltip;
  for (auto action : actions) {
    glm::vec4 color = bgcolor_;
    bool action_hover = hover_ && pointInBox(hoverPos_, center, actionSize_, 0);
    if (action_hover && action.state == UIAction::ENABLED) {
      float fact = press_ ? 0.8f : 1.2f;
      color *= glm::vec4(fact, fact, fact, 1.f);
    }
    drawRectCenter(center, actionSize_, color);
    drawTextureCenter(
        center,
        actionSize_ - glm::vec2(5.f),
        ResourceManager::get()->getTexture(action.icon),
        glm::vec4(0, 0, 1, 1));

    if (action.state == UIAction::COOLDOWN) {
      Shader *shader = ResourceManager::get()->getShader("cooldown");
      shader->makeActive();
      shader->uniform4f("texcoord", glm::vec4(0, 0, 1, 1));
      shader->uniform1f("cooldown_percent", action.cooldown);
      shader->uniform4f("color", glm::vec4(0, 0, 0, 0.5f));
      drawShaderCenter(center, actionSize_);
    } else if (action.state == UIAction::DISABLED) {
      drawRectCenter(center, actionSize_, glm::vec4(0, 0, 0, 0.5f));
    } else if (action.state == UIAction::UNAVAILABLE) {
      drawRectCenter(center, actionSize_, glm::vec4(0, 0, 0, 0.8f));
    }

    if (action_hover && action.radius > 0) {
      auto ent = GameEntity::cast(Renderer::get()->getEntity(action.render_id));
      if (ent) {
        auto pos = ent->getPosition(t) + glm::vec3(0, 0, 0.1f);
        glm::mat4 transform = glm::scale(
            glm::translate(glm::mat4(1.f), pos),
            glm::vec3(action.radius * 2.f));
        renderCircleColor(transform, glm::vec4(1.f), 1.5f);
      }
    }

    if (action_hover && hoverTimer_ > fltParam("local.tooltipDelay")) {
      tooltip = action.tooltip;
      if (action.hotkey) {
        tooltip += "\nHotkey: ";
        tooltip += action.hotkey;
      }
      draw_tooltip = true;
    }

    center.x += actionSize_.x;
  }
  if (draw_tooltip) {
    float tooltip_font_height = fltParam("local.tooltipFontHeight");
    float tooltip_width = actionSize_.x * 2.f;

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
  auto actions = actionsFunc_();
  size_ = glm::vec2(actions.size() * actionSize_.x, actionSize_.y);

  hover_ = pointInBox(pos, getCenter(), getSize(), 0.f);
  hoverPos_ = pos;

  if (!(buttons & MouseButton::LEFT)) {
    if (press_ & hover_) {
      int idx = 1 / actionSize_.x * (pos.x - center_.x + size_.x / 2);
      actionExecutor_(actions[idx]);
    }
    press_ = false;
  } else if (hover_) {
    press_ = true;
  }
}

};  // rts
