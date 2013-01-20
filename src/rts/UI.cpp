#define GLM_SWIZZLE_XYZW
#include "rts/UI.h"
#include <sstream>
#include <functional>
#include "common/Clock.h"
#include "common/ParamReader.h"
#include "rts/Actor.h"
#include "rts/Building.h"
#include "rts/Controller.h"
#include "rts/GameController.h"
#include "rts/Graphics.h"
#include "rts/FontManager.h"
#include "rts/Game.h"
#include "rts/Map.h"
#include "rts/Player.h"
#include "rts/Renderer.h"
#include "rts/ResourceManager.h"
#include "rts/Unit.h"
#include "rts/Widgets.h"

namespace rts {

UI* UI::instance_ = nullptr;


glm::vec2 UI::convertUIPos(const glm::vec2 &pos) {
  auto resolution = Renderer::get()->getResolution();
  return glm::vec2(
      pos.x < 0 ? pos.x + resolution.x : pos.x,
      pos.y < 0 ? pos.y + resolution.y : pos.y);
}

UI::UI()
  : chatActive_(false) {
  invariant(!instance_, "Multiple UIs?");
  instance_ = this;
  playerID_ = NULL_ID;
}

UI::~UI() {
  for (auto pair : widgets_) {
    delete pair.second;
  }
  instance_ = nullptr;
}

UIWidget* UI::getWidget(const std::string &name) {
  auto it = widgets_.find(name);
  if (it == widgets_.end()) {
    return nullptr;
  }
  return it->second;
}

void UI::addWidget(const std::string &name, UIWidget *widget) {
  widgets_[name] = widget;
}

bool UI::handleMousePress(const glm::vec2 &screenCoord) {
  for (auto&& pair : widgets_) {
    if (pair.second->isClick(screenCoord)) {
      if (pair.second->handleClick(screenCoord)) {
        return true;
      }
    }
  }

  return false;
}

void UI::clearWidgets() {
  for (auto pair : widgets_) {
    delete pair.second;
  }
  widgets_.clear();
}

void UI::render(float dt) {
  glDisable(GL_DEPTH_TEST);

  for (auto&& pair : widgets_) {
    pair.second->render(dt);
  }

  glEnable(GL_DEPTH_TEST);
}

void UI::renderEntity(
    const ModelEntity *e,
    const glm::mat4 &transform,
    float dt) {
  if (!e->hasProperty(GameEntity::P_ACTOR)) {
    return;
  }
  record_section("renderActorInfo");
  // TODO(zack): only render for actors currently on screen/visible
  auto localPlayer = (LocalPlayer*) Game::get()->getPlayer(playerID_);
  if (!localPlayer) {
    return;
  }
  /*
  if (localPlayer->isSelected(e->getID())) {
    // A bit of a hack here...
    auto finalTransform = glm::translate(
        transform,
        glm::vec3(0, 0, 0.1));
    renderCircleColor(finalTransform,
        glm::vec4(vec3Param("colors.selected"), 1.f));
  } else if (entityHighlights_.find(e->getID()) != entityHighlights_.end()) {
    // A bit of a hack here...
    auto finalTransform = glm::translate(
        transform,
        glm::vec3(0, 0, 0.1));
    renderCircleColor(finalTransform,
        glm::vec4(vec3Param("colors.targeted"), 1.f));
  }
  */

  glDisable(GL_DEPTH_TEST);
  auto ndc = getProjectionStack().current() * getViewStack().current() *
      transform * glm::vec4(0, 0, 0, 1);
  ndc /= ndc.w;
  auto resolution = Renderer::get()->getResolution();
  auto coord = (glm::vec2(ndc.x, -ndc.y) / 2.f + glm::vec2(0.5f)) * resolution;
  auto actor = (const Actor *)e;
  // First the unit placard.  For now just a square in the color of the owner,
  // eventually an icon denoting unit and damage type + upgrades.
  glm::vec2 size = vec2Param("hud.actor_card.dim");
  glm::vec2 pos = coord - vec2Param("hud.actor_card.pos");
  auto player = Game::get()->getPlayer(actor->getPlayerID());
  auto color = player ? player->getColor() : vec3Param("global.defaultColor");
  drawRectCenter(pos, size, glm::vec4(color, 0.7f));

  // Cap status
  if (actor->hasProperty(GameEntity::P_CAPPABLE)) {
    Building *building = (Building*)actor;
    if (building->getCap() > 0.f &&
        building->getCap() < building->getMaxCap()) {
      float capFact = glm::max(
          building->getCap() / building->getMaxCap(),
          0.f);
      glm::vec2 size = vec2Param("hud.actor_cap.dim");
      glm::vec2 pos = coord - vec2Param("hud.actor_cap.pos");
      // Black underneath
      drawRectCenter(pos, size, glm::vec4(0, 0, 0, 1));
      pos.x -= size.x * (1.f - capFact) / 2.f;
      size.x *= capFact;
      const glm::vec4 cap_color = vec4Param("hud.actor_cap.color");
      drawRectCenter(pos, size, cap_color);
    }
  } else {
    // Display the health bar
    // Health bar flashes white on red (instead of green on red) when it has
    // recently taken damage.
    glm::vec4 healthBarColor = glm::vec4(0, 1, 0, 1);
    float timeSinceDamage = Clock::secondsSince(actor->getLastTookDamage());
    if (timeSinceDamage < fltParam("hud.actor_health.flash_duration")) {
      healthBarColor = glm::vec4(1, 1, 1, 1);
    }

    float healthFact = glm::max(
        0.f,
        actor->getHealth() / actor->getMaxHealth());
    glm::vec2 size = vec2Param("hud.actor_health.dim");
    glm::vec2 pos = coord - vec2Param("hud.actor_health.pos");
    // Red underneath for max health
    drawRectCenter(pos, size, glm::vec4(1, 0, 0, 1));
    // Green on top for current health
    pos.x -= size.x * (1.f - healthFact) / 2.f;
    size.x *= healthFact;
    drawRectCenter(pos, size, healthBarColor);
  }

  auto queue = actor->getProductionQueue();
  if (!queue.empty()) {
    // display production bar
    float prodFactor = 1.f - queue.front().time / queue.front().max_time;
    glm::vec2 size = vec2Param("hud.actor_prod.dim");
    glm::vec2 pos = coord - vec2Param("hud.actor_prod.pos");
    // Purple underneath for max time
    drawRectCenter(pos, size, glm::vec4(0.5, 0, 1, 1));
    // Blue on top for time elapsed
    pos.x -= size.x * (1.f - prodFactor) / 2.f;
    size.x *= prodFactor;
    drawRectCenter(pos, size, glm::vec4(0, 0, 1, 1));
  }
  glEnable(GL_DEPTH_TEST);
}

void UI::renderChat(float dt) {
  const auto chatActive = instance_->chatActive_;
  const auto chatBuffer = instance_->chatBuffer_;

  auto&& messages = Game::get()->getChatMessages();
  auto&& displayTime = fltParam("ui.messages.chatDisplayTime");
  bool validMessage = !messages.empty() &&
      Clock::secondsSince(messages.back().time) < displayTime;

  if (validMessage || chatActive) {
    auto pos = convertUIPos(vec2Param("ui.messages.pos"));
    auto fontHeight = fltParam("ui.messages.fontHeight");
    auto messageHeight = fontHeight + fltParam("ui.messages.heightBuffer");
    auto height = intParam("ui.messages.max") * messageHeight;
    glm::vec2 startPos = pos - glm::vec2(0, height);
    auto size = glm::vec2(fltParam("ui.messages.backgroundWidth"), height);
    drawRect(startPos, size, vec4Param("ui.messages.backgroundColor"));
    int numMessages =
        std::min(intParam("ui.messages.max"), (int)messages.size());
    for (int i = 1; i <= numMessages; i++) {
      const ChatMessage &message = messages[messages.size() - i];
      glm::vec2 messagePos = pos;
      messagePos.y -= i * messageHeight;
      FontManager::get()->drawString(message.msg, messagePos, fontHeight);
    }
    if (chatActive) {
      drawRect(pos, glm::vec2(size.x, 1.2f * fontHeight),
          vec4Param("ui.messages.inputColor"));
      FontManager::get()->drawString(chatBuffer, pos, fontHeight);
    }
  }
}

void UI::renderDragRect(float dt) {
  glm::vec4 rect = ((GameController *)Renderer::get()->getController())
    ->getDragRect();
  if (rect == glm::vec4(HUGE_VAL)) {
    return;
  }
  // TODO(zack): make this color a param
  auto color = glm::vec4(0.2f, 0.6f, 0.2f, 0.3f);
  glm::vec2 start = rect.xy;
  glm::vec2 end = rect.zw;
  drawRect(start, end - start, color);
}
};  // rts
