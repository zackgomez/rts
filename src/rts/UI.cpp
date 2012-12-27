#define GLM_SWIZZLE_XYZW
#include "rts/UI.h"
#include "common/ParamReader.h"
#include "rts/Building.h"
#include "rts/Engine.h"
#include "rts/FontManager.h"
#include "rts/Game.h"
#include "rts/Map.h"
#include "rts/Player.h"
#include "rts/Renderer.h"
#include "rts/ResourceManager.h"
#include "rts/Unit.h"

namespace rts {

glm::vec2 convertUIPos(const glm::vec2 &pos) {
  const glm::vec2 resolution = vec2Param("local.resolution");
  return glm::vec2(
      pos.x < 0 ? pos.x + resolution.x : pos.x,
      pos.y < 0 ? pos.y + resolution.y : pos.y);
}

glm::vec2 worldToMinimap(const glm::vec3 &mapPos) {
  const glm::vec2 &mapSize = Game::get()->getMap()->getSize();
  const glm::vec2 minimapSize = vec2Param("ui.minimap.dim");
  const glm::vec2 minimapPos = convertUIPos(vec2Param("ui.minimap.pos"));
  glm::vec2 pos = mapPos.xy;
  pos.y *= -1;
  pos += mapSize / 2.f;
  pos *= minimapSize / mapSize;
  pos += minimapPos;
  return pos;
}

UI::UI()
  : chatActive_(false) {
  // TODO(zack): eventually make this preload some resources

  const char * texWidgetNames[] = {"ui.unitinfo", "ui.topbar"};

  for (std::string name : texWidgetNames) {
    auto pos = convertUIPos(vec2Param(name + ".pos"));
    auto size = vec2Param(name + ".dim");
    auto tex = strParam(name + ".texture");
    widgets_.push_back(new TextureWidget(pos, size, tex));
  }
}

UI::~UI() {
  for (auto widget : widgets_) {
    delete widget;
  }
  // TODO(zack): eventually make this signal it doesn't need the UI resources
}

void UI::render(float dt) {
  glDisable(GL_DEPTH_TEST);

  for (auto&& widget : widgets_) {
    widget->render(dt);
  }

  renderMinimap();
  renderChat();

  glEnable(GL_DEPTH_TEST);
}

void UI::renderChat() {
  auto&& messages = Renderer::get()->getChatMessages();
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
    minimapCoord[i] = worldToMinimap(terrainCoord);
  }
  drawLine(minimapCoord[0], minimapCoord[1], lineColor);
  drawLine(minimapCoord[1], minimapCoord[3], lineColor);
  drawLine(minimapCoord[3], minimapCoord[2], lineColor);
  drawLine(minimapCoord[2], minimapCoord[0], lineColor);

  // render actors
  for (const auto &pair : Renderer::get()->getEntityWorldPosMap()) {
    const Entity *e = (const Entity *)pair.first;
    if (e->getType() != Building::TYPE && e->getType() != Unit::TYPE) {
      continue;
    }
    const glm::vec3 &mapCoord = pair.second;
    glm::vec2 pos = worldToMinimap(mapCoord);
    const Player *player = Game::get()->getPlayer(e->getPlayerID());
    glm::vec3 pcolor = player ? player->getColor() :
      vec3Param("global.defaultColor");
    float actorSize = fltParam("ui.minimap.actorSize");
    drawRect(pos, glm::vec2(actorSize), glm::vec4(pcolor, 1.f));
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
};  // rts
