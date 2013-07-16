#include "rts/BorderWidget.h"
#include "common/Collision.h"

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
  //drawRectCenter(center_, size_, borderColor_, 0.f);
  child_->render(dt);
}

void BorderWidget::update(const glm::vec2 &pos, int buttons) {
  child_->setCenter(center_);
  child_->setSize(getInnerSize());

  child_->update(pos, buttons);
}

glm::vec2 BorderWidget::getInnerSize() const {
  return size_ - 2.f;
}

};  // rts
