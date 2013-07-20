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

BorderWidget *makePartWidget(
  const glm::vec2 &pos,
  const glm::vec2 &size,
  const glm::vec4 &bgcolor,
  const glm::vec4 &bordercolor,
  ActorPanelWidget::PartUpgradeHandler upgradeHandler) {

  auto *part_widget = new PartWidget(bgcolor);
  part_widget->setUpgradeHandler(upgradeHandler);
  auto *widget = new BorderWidget(
    bordercolor,
    new TooltipWidget(part_widget));
  widget->setCenter(pos);
  widget->setSize(size);
  return widget;
}

ActorPanelWidget::ActorPanelWidget(
    const std::string &name,
    ActorFunc f,
    PartUpgradeHandler upgrade_handler)
  : name_(name),
    bgcolor_(vec4Param(name + ".bgcolor")),
    actorFunc_(f) {
  center_ = uiPosParam(name + ".center");
  size_ = uiSizeParam(name + ".dim");

  float health_height = fltParam(name + ".health.height_factor") * 2.f * size_.y;

  // TODO(zack): this will have issues with dynamic resizing
  glm::vec2 part_area_center = glm::vec2(center_.x, center_.y - health_height / 2.f);
  glm::vec2 part_area_size = glm::vec2(size_.x, size_.y - health_height);
  auto part_bgcolor = vec4Param(name + ".parts.bgcolor");
  auto part_bordercolor = vec4Param(name + ".parts.bordercolor");
  const glm::vec2 part_size(
      part_area_size.x * fltParam(name_ + ".parts.width_factor"),
      part_area_size.y * fltParam(name_ + ".parts.height_factor"));
  // corners
  for (int i = 0; i < 2; i++) {
    for (int j = 0; j < 2; j++) {
      float x = i % 2 == 0
        ? part_area_center.x - part_area_size.x / 2.f + part_size.x / 2.f
        : part_area_center.x + part_area_size.x / 2.f - part_size.x / 2.f;
      float y = j % 2 == 0
        ? part_area_center.y - part_area_size.y / 2.f + part_size.y / 2.f
        : part_area_center.y + part_area_size.y / 2.f - part_size.y / 2.f;

      partWidgets_.push_back(makePartWidget(
        glm::vec2(x, y),
        part_size,
        part_bgcolor,
        part_bordercolor,
        upgrade_handler));
    }
  }

  // middle two
  for (int i = 0; i < 2; i++) {
    float sign = i ? 1 : -1;
    float x = part_area_center.x + sign * part_area_size.x / 2.f - sign * part_size.x / 2.f;
    float y = part_area_center.y;

    partWidgets_.push_back(makePartWidget(
      glm::vec2(x, y),
      part_size,
      part_bgcolor,
      part_bordercolor,
      upgrade_handler));
  }
}
 
ActorPanelWidget::~ActorPanelWidget() {
  for (auto *part : partWidgets_) {
    delete part;
  }
}

bool ActorPanelWidget::handleClick(const glm::vec2 &pos, int button) {
  auto *actor = actorFunc_
    ? actorFunc_()
    : nullptr;
  if (!actor) {
    return false;
  }

  auto ui_info = actor->getUIInfo();
  for (int i = 0; i < partWidgets_.size() && i < ui_info.parts.size(); i++) {
    auto *widget = partWidgets_[i];
    if (pointInBox(pos, widget->getCenter(), widget->getSize(), 0.f)) {
      if (widget->handleClick(pos, button)) {
        break;
      }
    }
  }
  return true;
}

void renderBarWithText(
  const glm::vec2 &pos,
  const glm::vec2 &size,
  const glm::vec4 &bgcolor,
  const glm::vec4 &color,
  const glm::vec2 &amounts) {

  // background
  drawRectCenter(
    pos,
    size,
    bgcolor);

  float max_health = amounts[1];
  invariant(max_health != 0.f, "cannot do 0 max health bar");
  float health = glm::clamp(amounts[0], 0.f, max_health);
  float fact = health / max_health;
  glm::vec2 fg_size(
    size.x * fact,
    size.y);
  glm::vec2 fg_center(
    pos.x - size.x / 2.f + fg_size.x / 2.f,
    pos.y);
  drawRectCenter(fg_center, fg_size, color);

  // draw amount
  std::stringstream ss;
  ss << glm::floor(health) << " / " << glm::floor(max_health);
  FontManager::get()->drawString(
      ss.str(),
      glm::vec2(pos.x - size.x / 2.f, pos.y - size.y / 2.f),
      0.9f * size.y);
}

void ActorPanelWidget::render(float dt) {
  auto *actor = actorFunc_ ? actorFunc_() : nullptr;
  if (!actor) {
    return;
  }
  auto ui_info = actor->getUIInfo();

  // Background
  drawRectCenter(center_, size_, bgcolor_);
  // Part widgets
  glm::vec2 total_health(0.f);
  for (int i = 0; i < ui_info.parts.size(); i++) {
    partWidgets_[i]->render(dt);
    total_health += glm::max(glm::vec2(0.f), ui_info.parts[i].health);
  }

  // Health and mana bars
  float health_height = fltParam(name_ + ".health.height_factor") * size_.y;
  glm::vec2 bar_center(
    center_.x,
    center_.y + size_.y / 2.f - health_height / 2.f);
  glm::vec2 bar_size(size_.x, health_height);
  std::tuple<std::string, glm::vec2> bars[] = {
    make_tuple(std::string(".mana"), ui_info.mana),
    make_tuple(std::string(".health"), total_health),
  };

  for (const auto &bar : bars) {
    auto bar_amount = std::get<1>(bar);
    if (bar_amount[1] > 0.f) {
      renderBarWithText(
        bar_center,
        bar_size,
        vec4Param(name_ + std::get<0>(bar) + ".bgcolor"),
        vec4Param(name_ + std::get<0>(bar) + ".color"),
        bar_amount);
      bar_center.y -= health_height;
    }
  }
}

void ActorPanelWidget::update(const glm::vec2 &pos, int buttons) {
  auto *actor = actorFunc_ ? actorFunc_() : nullptr;
  if (!actor) {
    return;
  }
  auto ui_info = actor->getUIInfo();
  auto nparts = ui_info.parts.size();
  invariant(nparts <= partWidgets_.size(), "too many parts to render");

  for (int i = 0; i < partWidgets_.size(); i++) {
    auto *widget = partWidgets_[i];
    widget->setUI(getUI());

    auto *tooltipWidget = (TooltipWidget *)widget->getChild();
    auto *partWidget = ((PartWidget *)tooltipWidget->getChild());
    if (i < nparts) {
      tooltipWidget->setTooltipFunc([=]() -> std::string {
        return ui_info.parts[i].name + ":\n" + ui_info.parts[i].tooltip;
      });
      partWidget->setPart(ui_info.parts[i]);
    } else {
      tooltipWidget->setTooltipFunc(TooltipWidget::TooltipFunc());
    }
    widget->update(pos, buttons);
  }
}

bool PartWidget::handleClick(const glm::vec2 &pos, int button) {
  if (part_.upgrades.size()) {
    // TODO(zack): this should be updated
    upgradeHandler_(part_.upgrades.front());
  }
  return true;
}

void PartWidget::render(float dt) {
  drawRectCenter(center_, size_, bgcolor_, 0.f);

  if (part_.health[1] == 0.f) {
    return;
  }
  float health_height = size_.y / 5.f;
  float y = center_.y + size_.y / 2.f - health_height / 2.f;
  renderBarWithText(
    glm::vec2(center_.x, y),
    glm::vec2(size_.x, health_height),
    glm::vec4(0.f, 0.f, 0.f, 1.f),
    glm::vec4(0.9f, 0.9f, 0.1f, 1.f),
    part_.health);

  float upgrade_size = 0.2f * glm::max(size_.x, size_.y);
  if (part_.upgrades.size()) {
    drawRectCenter(
        glm::vec2(
          center_.x + size_.x / 2.f - upgrade_size / 2.f,
          center_.y - size_.y / 2.f + upgrade_size / 2.f),
        glm::vec2(upgrade_size),
        glm::vec4(0.8, 0.2, 0.2, 1.0));
  }
}

};  // rts
