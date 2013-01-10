#define GLM_SWIZZLE_XYZW
#include "rts/UI.h"
#include <sstream>
#include "common/Clock.h"
#include "common/ParamReader.h"
#include "rts/Actor.h"
#include "rts/Building.h"
#include "rts/Graphics.h"
#include "rts/FontManager.h"
#include "rts/Game.h"
#include "rts/Map.h"
#include "rts/Player.h"
#include "rts/Renderer.h"
#include "rts/ResourceManager.h"
#include "rts/Unit.h"

namespace rts {

glm::vec2 worldToMinimap(glm::vec2 pos) {
  const glm::vec2 &mapSize = Game::get()->getMap()->getSize();
  const glm::vec2 minimapSize = vec2Param("ui.minimap.dim");
  const glm::vec2 minimapPos = UI::convertUIPos(vec2Param("ui.minimap.pos"));
  pos.y *= -1;
  pos += mapSize / 2.f;
  pos *= minimapSize / mapSize;
  pos += minimapPos;
  return pos;
}

glm::vec2 UI::convertUIPos(const glm::vec2 &pos) {
  auto resolution = Renderer::get()->getResolution();
  return glm::vec2(
      pos.x < 0 ? pos.x + resolution.x : pos.x,
      pos.y < 0 ? pos.y + resolution.y : pos.y);
}

UI::UI()
  : chatActive_(false),
    dragStart_(HUGE_VAL),
    dragEnd_(HUGE_VAL) {
}

UI::~UI() {
  for (auto widget : widgets_) {
    delete widget;
  }
}

void UI::initGameWidgets(id_t playerID) {
  // TODO(zack): remove this, put it in a widget
  playerID_ = playerID;

  const char * texWidgetNames[] = {"ui.unitinfo", "ui.topbar"};
  for (std::string name : texWidgetNames) {
    auto pos = convertUIPos(vec2Param(name + ".pos"));
    auto size = vec2Param(name + ".dim");
    auto tex = strParam(name + ".texture");
    widgets_.push_back(new TextureWidget(pos, size, tex));
  }

  {
    std::string name = "ui.reqdisplay";
    auto pos = convertUIPos(vec2Param(name + ".pos"));
    auto size = vec2Param(name + ".dim");
    auto fontHeight = fltParam(name + ".fontHeight");
    auto bgcolor = vec4Param(name + ".bgcolor");
    auto textFunc = [=]()->std::string {
      std::stringstream ss;
      ss << "Req: "
         << (int)Game::get()->getResources(playerID).requisition;
      return ss.str();
    };
    widgets_.push_back(new TextWidget<decltype(textFunc)>(
          pos, size, fontHeight, bgcolor, textFunc));
  }

  {
    auto pos = convertUIPos(vec2Param("ui.vicdisplay.pos"));
    auto size = vec2Param("ui.vicdisplay.dim");
    auto fontHeight = fltParam("ui.vicdisplay.fontHeight");
    for (id_t tid : Game::get()->getTeams()) {
      // background in team color
      int col_idx = tid - STARTING_TID;
      glm::vec4 bgcolor = glm::vec4(
          toVec3(getParam("colors.team")[col_idx]), 1.f);
      auto textFunc = [=]()->std::string {
        std::stringstream ss;
        ss << (int)Game::get()->getVictoryPoints(tid);
        return ss.str();
      };
      widgets_.push_back(new TextWidget<decltype(textFunc)>(
            pos, size, fontHeight, bgcolor, textFunc));

      pos.x += size.x * 2.0;
    }
  }
}

void UI::render(float dt) {
  renderHighlights(dt);

  glDisable(GL_DEPTH_TEST);

  for (auto&& widget : widgets_) {
    widget->render(dt);
  }

  renderDragRect(dt);
  renderMinimap();
  renderChat();

  glEnable(GL_DEPTH_TEST);
}

void UI::setDragRect(const glm::vec3 &s, const glm::vec3 &e) {
  dragStart_ = s;
  dragEnd_ = e;
}

void UI::highlight(const glm::vec2 &mapCoord) {
  MapHighlight hl;
  hl.pos = mapCoord;
  hl.remaining = fltParam("ui.highlight.duration");
  highlights_.push_back(hl);
}

void UI::highlightEntity(id_t eid) {
  entityHighlights_[eid] = fltParam("ui.highlight.duration");
}

void UI::renderEntity(const ModelEntity *e, const glm::mat4 &transform, float dt) {
  if (!e->hasProperty(GameEntity::P_ACTOR)) {
    return;
  }
  record_section("renderActorInfo");
  // TODO(zack): only render for actors currently on screen/visible
  auto localPlayer = (LocalPlayer*) Game::get()->getPlayer(playerID_);
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

void UI::renderChat() {
  auto&& messages = Game::get()->getChatMessages();
  auto&& displayTime = fltParam("ui.messages.chatDisplayTime");
  bool validMessage = !messages.empty() &&
      Clock::secondsSince(messages.back().time) < displayTime;

  if (validMessage || chatActive_) {
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
    if (chatActive_) {
      drawRect(pos, glm::vec2(size.x, 1.2f * fontHeight),
          vec4Param("ui.messages.inputColor"));
      FontManager::get()->drawString(chatBuffer_, pos, fontHeight);
    }
  }
}

void UI::renderMinimap() {
  const glm::vec2 &mapSize = Game::get()->getMap()->getSize();
  const glm::vec4 &mapColor = Game::get()->getMap()->getMinimapColor();
  // TODO(connor) we probably want some small buffer around the sides of the
  // minimap so we can see the underlay image..
  const glm::vec2 minimapSize = vec2Param("ui.minimap.dim");
  const glm::vec2 minimapPos = convertUIPos(vec2Param("ui.minimap.pos"));

  // TODO(connor) support other aspect ratios so they don't stretch or distort

  // Render base image
  drawRect(minimapPos, minimapSize, mapColor);

  // Render viewport
  glm::vec2 res = vec2Param("local.resolution");
  glm::vec4 lineColor = vec4Param("ui.minimap.viewportLineColor");

  // NOTE(zack): this is a hack to get the viewport to render correctly in
  // windows.  If offset is 0, the coordinates get wonky.
  float offset = 1;
  glm::vec2 screenCoords[] = {
    glm::vec2(offset, offset),
    glm::vec2(offset, res.y - offset),
    glm::vec2(res.x - offset, offset),
    glm::vec2(res.x - offset, res.y - offset),
  };
  // Iterates over the four corners of the current viewport
  glm::vec2 minimapCoord[4];
  for (int i = 0; i < 4; i++) {
    glm::vec2 screenCoord = screenCoords[i];
    glm::vec3 terrainCoord = Renderer::get()->screenToTerrain(screenCoord);
    if (terrainCoord.x == HUGE_VAL) terrainCoord.x = mapSize.x / 2;
    if (terrainCoord.x == -HUGE_VAL) terrainCoord.x = -mapSize.x / 2;
    if (terrainCoord.y == HUGE_VAL) terrainCoord.y = mapSize.y / 2;
    if (terrainCoord.y == -HUGE_VAL) terrainCoord.y = -mapSize.y / 2;
    minimapCoord[i] = worldToMinimap(glm::vec2(terrainCoord));
  }
  drawLine(minimapCoord[0], minimapCoord[1], lineColor);
  drawLine(minimapCoord[1], minimapCoord[3], lineColor);
  drawLine(minimapCoord[3], minimapCoord[2], lineColor);
  drawLine(minimapCoord[2], minimapCoord[0], lineColor);

  // render actors
  for (const auto &pair : Game::get()->getEntities()) {
    const GameEntity *e = (const GameEntity *)pair.second;
    if (!e->hasProperty(GameEntity::P_ACTOR)) {
      continue;
    }
    glm::vec2 pos = worldToMinimap(e->getPosition(Renderer::get()->getSimDT()));
    const Player *player = Game::get()->getPlayer(e->getPlayerID());
    glm::vec3 pcolor = player ? player->getColor() :
      vec3Param("global.defaultColor");
    float actorSize = fltParam("ui.minimap.actorSize");
    drawRect(pos, glm::vec2(actorSize), glm::vec4(pcolor, 1.f));
  }
}

void UI::renderHighlights(float dt) {
  // Render each of the highlights
  for (auto& hl : highlights_) {
    hl.remaining -= dt;
    glm::mat4 transform =
      glm::scale(
          glm::translate(
            glm::mat4(1.f),
            glm::vec3(hl.pos.x, hl.pos.y, 0.01f)),
          glm::vec3(0.33f));
    renderCircleColor(transform, glm::vec4(1, 0, 0, 1));
  }
  // Remove done highlights
  for (size_t i = 0; i < highlights_.size(); ) {
    if (highlights_[i].remaining <= 0.f) {
      std::swap(highlights_[i], highlights_[highlights_.size() - 1]);
      highlights_.pop_back();
    } else {
      i++;
    }
  }

  auto it = entityHighlights_.begin();
  while (it != entityHighlights_.end()) {
    auto cur = (entityHighlights_[it->first] -= dt);
    if (cur <= 0.f) {
      it = entityHighlights_.erase(it);
    } else {
      it++;
    }
  }
}

void UI::renderDragRect(float dt) {
  // Render drag selection if it exists
  if (dragStart_ != glm::vec3(HUGE_VAL)) {
    glm::mat4 fullmat =
      getProjectionStack().current() * getViewStack().current();
    const auto resolution = vec2Param("local.resolution");

    glm::vec2 start = applyMatrix(fullmat, dragStart_).xy;
    glm::vec2 end = applyMatrix(fullmat, dragEnd_).xy;
    start = (glm::vec2(start.x, -start.y) + glm::vec2(1.f))
        * 0.5f * resolution;
    end = (glm::vec2(end.x, -end.y) + glm::vec2(1.f))
        * 0.5f * resolution;

    // TODO(zack): make this color a param
    drawRectCenter((start + end) / 2.f, end - start,
        glm::vec4(0.2f, 0.6f, 0.2f, 0.3f));
    // Reset each frame
    dragStart_ = glm::vec3(HUGE_VAL);
  }
}

TextureWidget::TextureWidget(
    const glm::vec2 &pos,
    const glm::vec2 &size,
    const std::string &texName)
  : pos_(pos),
    size_(size),
    texName_(texName) {
}

void TextureWidget::render(float dt) {
  auto tex = ResourceManager::get()->getTexture(texName_);
  drawTexture(pos_, size_, tex);
}

template<class T>
TextWidget<T>::TextWidget(
    const glm::vec2 &pos,
    const glm::vec2 &size,
    float height,
    const glm::vec4 &bgcolor,
    T& textGetter)
  : pos_(pos),
    size_(size),
    height_(height),
    bgcolor_(bgcolor),
    textFunc_(textGetter) {
}

template<class T>
void TextWidget<T>::render(float dt) {
  const std::string text = textFunc_();
  drawRect(pos_, size_, bgcolor_);
  FontManager::get()->drawString(text, pos_, height_);
}
};  // rts
