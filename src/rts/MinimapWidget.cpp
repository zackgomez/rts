#include "rts/MinimapWidget.h"
#include "common/ParamReader.h"
#include "rts/Game.h"
#include "rts/Graphics.h"
#include "rts/Player.h"
#include "rts/Map.h"
#include "rts/Renderer.h"
#include "rts/UI.h"

namespace rts {

MinimapWidget::MinimapWidget(const std::string &name, id_t localPlayerID)
  : StaticWidget(name),
    localPlayerID_(localPlayerID),
    name_(name) {

  glGenTextures(1, &visibilityTex_);
  glBindTexture(GL_TEXTURE_2D, visibilityTex_);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

MinimapWidget::~MinimapWidget() {
  glDeleteTextures(1, &visibilityTex_);
}

bool MinimapWidget::handleClick(const glm::vec2 &pos, int button) {
  if (listener_) {
    glm::vec2 minimap_pos = (pos - getCenter()) / getSize();
    listener_(minimap_pos, button);
  }

  return true;
}

void MinimapWidget::renderBase() {
  auto mapColor = Game::get()->getMap()->getColor();

  auto shader = ResourceManager::get()->getShader("minimap");
  shader->makeActive();
  shader->uniform4f("color", mapColor);
  shader->uniform1f("texture", 0);
  shader->uniform4f("texcoord", glm::vec4(0, 1, 1, 0));

  // TODO(zack): fill visibilityTex_

  glActiveTexture(GL_TEXTURE0);
  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, visibilityTex_);

  drawShaderCenter(getCenter(), getSize());
}

void MinimapWidget::render(float dt) {
  const float t = Renderer::get()->getGameTime();
  // TODO(connor) we probably want some small buffer around the sides of the
  // minimap so we can see the underlay image..

  // TODO(connor) support other aspect ratios so they don't stretch or distort

  // Render base image
  renderBase();

  // Render viewport
  /*
  const glm::vec2 &mapSize = Game::get()->getMap()->getSize();
  // NOTE(zack): this is a hack to get the viewport to render correctly in
  // windows.  If offset is 0, the coordinates get wonky.
  glm::vec4 lineColor = vec4Param(name_ + ".viewportLineColor");
  glm::vec2 res = vec2Param("local.resolution");
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
  */

  int colorScheme = intParam("local.minimap_color_scheme"); 
    //0 = player colors (red player is red...)
    //1 = ally (you are green, allies blue, enemies red...)
  
  const LocalPlayer *localPlayer = (LocalPlayer *)Game::get()->getPlayer(localPlayerID_);
  id_t teamID_ = localPlayer->getTeamID();
    
  // render actors
  for (const auto &pair : Renderer::get()->getEntities()) {
    if (!pair.second->hasProperty(GameEntity::P_ACTOR)) {
      continue;
    }
    const GameEntity *e = (const GameEntity *)pair.second;
    if (!e->isVisible()) {
      continue;
    }
    glm::vec2 pos = worldToMinimap(e->getPosition(t));
    const Player *player = Game::get()->getPlayer(e->getPlayerID(t));
    glm::vec3 pcolor; 
    if (colorScheme == 0) {
      pcolor = player ? player->getColor() : vec3Param("global.defaultColor");
    } else {
      if (!player) { //no player
        pcolor =  vec3Param("colors.minimap.neutral");
      } else if (localPlayer->isSelected(e->getGameID())) { //selected
        pcolor = vec3Param("colors.minimap.local_selected");
      } else if (player->getPlayerID() == localPlayerID_) { //local player
        pcolor = vec3Param("colors.minimap.local");
      } else if (player->getTeamID() == teamID_) { //on same team
        pcolor = vec3Param("colors.minimap.ally");
      } else { //enemy
        pcolor = vec3Param("colors.minimap.enemy");
      }
    }
    std::string icon_name = e->getUIInfo().minimap_icon;
    glm::vec2 actorSize = glm::vec2(fltParam(name_ + ".actorSize"));
    if (!icon_name.empty()) {
      drawTextureCenter(
          pos,
          actorSize,
          ResourceManager::get()->getTexture(icon_name),
          glm::vec4(0, 0, 1, 1),
          glm::vec4(pcolor, 1.f));
    } else {
      drawRectCenter(pos, actorSize, glm::vec4(pcolor, 1.f));
    }
  }
}

glm::vec2 MinimapWidget::worldToMinimap(const glm::vec3 &worldPos) const {
  const glm::vec2 &mapSize = Game::get()->getMap()->getSize();
  const glm::vec2 minimapSize = getSize();
  const glm::vec2 minimapPos = getCenter() - minimapSize/2.f;
  glm::vec2 pos = glm::vec2(worldPos);
  pos.y *= -1;
  pos += mapSize / 2.f;
  pos *= minimapSize / mapSize;
  pos += minimapPos;
  return pos;
}
};  // rts
