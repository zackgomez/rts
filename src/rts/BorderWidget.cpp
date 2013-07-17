#include "rts/BorderWidget.h"
#include <boost/algorithm/string.hpp>
#include "common/Collision.h"
#include "common/ParamReader.h"
#include "rts/UI.h"

namespace rts {

BorderWidget::BorderWidget(
    const glm::vec4 &border_color,
    UIWidget *child)
  : borderColor_(border_color),
    child_(child) {
}

BorderWidget::~BorderWidget() {
  delete child_;
}

bool BorderWidget::handleClick(const glm::vec2 &pos, int button) {
  if (pointInBox(pos, child_->getCenter(), child_->getSize(), 0.f)) {
    return child_->handleClick(pos, button);
  }
  return true;
}

void BorderWidget::render(float dt) {
  drawRectCenter(center_, size_, borderColor_, 0.f);
  child_->render(dt);
}

void BorderWidget::update(const glm::vec2 &pos, int buttons) {
  child_->setUI(getUI());
  child_->setCenter(center_);
  child_->setSize(getInnerSize());

  child_->update(pos, buttons);
}

glm::vec2 BorderWidget::getInnerSize() const {
  return size_ - 2.f;
}

TooltipWidget::TooltipWidget(UIWidget *child)
  : child_(child) {
}

TooltipWidget::~TooltipWidget() {
  delete child_;
}

bool TooltipWidget::handleClick(const glm::vec2 &pos, int button) {
  if (pointInBox(pos, child_->getCenter(), child_->getSize(), 0.f)) {
    return child_->handleClick(pos, button);
  }
  return false;
}

void TooltipWidget::render(float dt) {
  child_->render(dt);
}

void TooltipWidget::update(const glm::vec2 &pos, int buttons) {
  child_->setUI(getUI());
  child_->setCenter(center_);
  child_->setSize(size_);

  child_->update(pos, buttons);

  if (!pointInBox(pos, center_, size_, 0.f)) {
    return;
  }

  auto tooltip = tooltipFunc_();
  if (tooltip.empty()) {
    return;
  }

  std::vector<std::string> tooltipLines;
  boost::split(tooltipLines, tooltip, boost::is_any_of("\n"));
  float tooltip_font_height = fltParam("local.tooltipFontHeight");
  getUI()->addDeferedRenderFunc([=]() -> void {
    glm::vec2 tooltip_pos = pos -
      glm::vec2(0.f, (tooltipLines.size() + 1) * tooltip_font_height);

    float tooltip_font_height = fltParam("local.tooltipFontHeight");
    // TODO(zack): calculate this from the tooltip string
    float tooltip_width = 50.f;

    glm::vec2 tooltip_rect_size = glm::vec2(
      tooltip_width,
      tooltip_font_height * (tooltipLines.size() + 0.5f));

    drawRect(tooltip_pos, tooltip_rect_size, glm::vec4(0.6f));
  });
}

};  // rts
