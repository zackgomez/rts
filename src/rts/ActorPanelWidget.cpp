#include "rts/ActorPanelWidget.h"
#include <sstream>
#include <SDL/SDL.h>
#include "common/ParamReader.h"
#include "rts/Actor.h"
#include "rts/BorderWidget.h"
#include "rts/FontManager.h"
#include "rts/Game.h"
#include "rts/Renderer.h"

namespace rts {

ActorPanelWidget::ActorPanelWidget(
    const std::string &name,
    ActorFunc f)
  : name_(name),
    bgcolor_(vec4Param(name + ".bgcolor")),
    actorFunc_(f),
    hidden_(false) {
  center_ = uiPosParam(name + ".center");
  size_ = uiSizeParam(name + ".dim");

  for (int i = 0; i < 6; i++) {
    partWidgets_.push_back(new BorderWidget(
          glm::vec4(0, 0, 0, 1),
          new PartWidget()));
  }
}
 
ActorPanelWidget::~ActorPanelWidget() {
  for (auto *part : partWidgets_) {
    delete part;
  }
}

bool ActorPanelWidget::handleClick(const glm::vec2 &pos, int button) {
  return actorFunc_ && actorFunc_();
}

void ActorPanelWidget::render(float dt) {
  if (hidden_) {
    return;
  }
  drawRectCenter(center_, size_, bgcolor_);
  for (int i = 0; i < partWidgets_.size(); i++) {
    partWidgets_[i]->render(dt);
  }
}

void ActorPanelWidget::update(const glm::vec2 &pos, int buttons) {
  auto *actor = actorFunc_ ? actorFunc_() : nullptr;
  hidden_ = !actor;
  if (hidden_) {
    return;
  }
  auto ui_info = actor->getUIInfo();
  invariant(ui_info.parts.size() <= 6, "too many parts to render");
  const glm::vec2 part_size(
      size_.x * 0.25f,
      size_.y / 3.f);
  for (int i = 0; i < partWidgets_.size(); i++) {
    float x = i % 2 == 0
      ? center_.x - size_.x / 2.f + part_size.x / 2.f
      : center_.x + size_.x / 2.f - part_size.x / 2.f;
    float y = center_.y - size_.y / 2.f + part_size.y * (0.5f + i / 2);

    auto *widget = partWidgets_[i];
    widget->setCenter(glm::vec2(x, y));
    widget->setSize(part_size);

    if (i < ui_info.parts.size()) {
      ((PartWidget *)widget->getChild())->setPart(ui_info.parts[i]);

      widget->update(pos, buttons);
    } else {
      widget->setSize(glm::vec2(0.f));
    }
  }
}

bool PartWidget::handleClick(const glm::vec2 &pos, int button) {
  return true;
}

void PartWidget::render(float dt) {
  drawRectCenter(center_, size_, glm::vec4(1,1,1,1), 0.f);

  float health = std::max(0.f, part.health[0]);

  float fact = health / part.health[1];
  float health_height = inside_size.y / 5.f;
  float y = center.y + inside_size.y / 2.f - health_height / 2.f;
  drawRectCenter(
      glm::vec2(center_.x, y),
      glm::vec2(size_.x, health_height),
      glm::vec4(0.8, 0.2, 0.2, 1.0),
      0.f);
  drawRectCenter(
      glm::vec2(center_.x - size_.x / 2.f + size_.x * fact / 2.f, y),
      glm::vec2(size_.x * fact, health_height),
      glm::vec4(0.2, 0.9, 0.2, 1.0),
      0.f);
  std::stringstream ss;
  ss << glm::floor(health) << " / " << glm::floor(part.health[1]);
  FontManager::get()->drawString(
      ss.str(),
      glm::vec2(center_.x - size_.x / 2.f, y - health_height / 2.f),
      health_height);
}

};  // rts
