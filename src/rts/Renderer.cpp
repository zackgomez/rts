#define GLM_SWIZZLE_XYZW
#include "rts/Renderer.h"
#include <SDL/SDL.h>
#include <iostream>
#include <GL/glew.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>
#include "common/ParamReader.h"
#include "common/util.h"
#include "rts/Building.h"
#include "rts/Engine.h"
#include "rts/Entity.h"
#include "rts/FontManager.h"
#include "rts/Game.h"
#include "rts/Map.h"
#include "rts/MessageHub.h"
#include "rts/Projectile.h"
#include "rts/Player.h"
#include "rts/ResourceManager.h"
#include "rts/Unit.h"

namespace rts {

LoggerPtr Renderer::logger_;

Renderer::Renderer()
  : game_(NULL),
    player_(NULL),
    cameraPos_(0.f, 0.f, 5.f),
    resolution_(vec2Param("local.resolution")),
    dragStart_(HUGE_VAL),
    dragEnd_(HUGE_VAL),
    displayChatBoxTimer_(0.f),
    lastRender_(0) {
  if (!logger_.get()) {
    logger_ = Logger::getLogger("OGLRenderer");
  }

  initEngine(resolution_);
  // Initialize font manager, if necessary
  FontManager::get();

  // unit model is based at 0, height 1, translate to center of model
  glm::mat4 unitMeshTrans = glm::scale(glm::mat4(1.f), glm::vec3(1, 0.5f, 1));
  setMeshTransform(ResourceManager::get()->getMesh("unit"), unitMeshTrans);
  setMeshTransform(
    ResourceManager::get()->getMesh("melee_unit"),
    glm::scale(unitMeshTrans, glm::vec3(2.f)));

  glm::mat4 projMeshTrans =
    glm::rotate(glm::mat4(1.f), 90.f, glm::vec3(1, 0, 0));
  setMeshTransform(
    ResourceManager::get()->getMesh("basic_bullet"),
    projMeshTrans);
}

Renderer::~Renderer() {
  for (Effect *effect : effects_) {
    delete effect;
  }
}

void Renderer::renderMessages(const std::set<Message> &messages) {
  for (const Message &msg : messages) {
    if (msg["type"] == MessageTypes::ATTACK) {
      for (Json::Value _victim : msg["to"]) {
        id_t victim = toID(_victim);
        effects_.push_back(new BloodEffect(victim));
      }
    }
  }
}

void Renderer::renderEntity(const Entity *entity) {
  glm::vec3 pos = glm::vec3(entity->getPosition(simdt_), 0);
  float rotAngle = entity->getAngle(simdt_);
  const std::string &type = entity->getType();
  float modelSize = fltParam(entity->getName() + ".modelSize");

  // Interpolate if they move
  glm::mat4 transform =
      glm::scale(
        glm::rotate(
          glm::rotate(
            glm::translate(glm::mat4(1.f), pos),
            rotAngle, glm::vec3(0, 0, 1)),
          90.f, glm::vec3(1, 0, 0)),
        glm::vec3(modelSize));

  if (type == Unit::TYPE || type == Building::TYPE) {
    renderActor((const Actor *) entity, transform);
  } else if (type == Projectile::TYPE) {
    // TODO(zack): move to renderProjectile
    // TODO(zack): make this color to a param in Projectile
    glm::vec4 color = glm::vec4(0.5, 0.7, 0.5, 1);
    GLuint meshProgram = ResourceManager::get()->getShader("unit");
    glUseProgram(meshProgram);
    GLuint colorUniform = glGetUniformLocation(meshProgram, "color");
    GLuint lightPosUniform = glGetUniformLocation(meshProgram, "lightPos");
    glUniform4fv(colorUniform, 1, glm::value_ptr(color));
    glUniform3fv(lightPosUniform, 1, glm::value_ptr(lightPos_));
    Mesh * mesh =
      ResourceManager::get()->getMesh(strParam(entity->getName() + ".model"));
    renderMesh(transform, mesh);
  } else {
    LOG(WARNING) << "Unable to render entity " << entity->getName() << '\n';
  }
}

glm::vec2 Renderer::convertUIPos(const glm::vec2 &pos) {
  return glm::vec2(
      pos.x < 0 ? pos.x + resolution_.x : pos.x,
      pos.y < 0 ? pos.y + resolution_.y : pos.y);
}

glm::vec2 Renderer::worldToMinimap(const glm::vec3 &mapPos) {
  const glm::vec2 &mapSize = game_->getMap()->getSize();
  const glm::vec2 minimapSize = vec2Param("ui.minimap.dim");
  const glm::vec2 minimapPos = convertUIPos(vec2Param("ui.minimap.pos"));
  glm::vec2 pos = mapPos.xy;
  pos.y *= -1;
  pos += mapSize / 2.f;
  pos *= minimapSize / mapSize;
  pos += minimapPos;
  return pos;
}

void Renderer::renderUI() {
  glDisable(GL_DEPTH_TEST);

  // TODO(connor) there may be a better way to do this
  // perhaps store the names of all the UI elements in an array
  // and iterate over it?

  // top bar:
  glm::vec2 pos = convertUIPos(vec2Param("ui.topbar.pos"));
  glm::vec2 size = vec2Param("ui.topbar.dim");
  GLuint tex =
    ResourceManager::get()->getTexture(strParam("ui.topbar.texture"));
  drawTexture(pos, size, tex);

  // Requestion display
  pos = convertUIPos(vec2Param("ui.reqdisplay.pos"));
  size = vec2Param("ui.reqdisplay.size");
  float height = fltParam("ui.reqdisplay.fontHeight");
  std::stringstream ss;
  ss << "Req: " << (int)game_->getResources(player_->getPlayerID()).requisition;
  drawRect(pos, size, vec4Param("ui.reqdisplay.bgcolor"));
  FontManager::get()->drawString(ss.str(), pos, height);

  // Victory points
  pos = convertUIPos(vec2Param("ui.vicdisplay.pos"));
  size = vec2Param("ui.vicdisplay.size");
  height = fltParam("ui.vicdisplay.fontHeight");
  // TODO(connor) this should probably iterate over teams, not players, but we
  // dont have teams yet..
  for (id_t tid : game_->getTeams()) {
    // background in team color
    int col_idx = tid - STARTING_TID;
    glm::vec3 tcol = toVec3(getParam("colors.team")[col_idx]);
    drawRect(pos, size, glm::vec4(tcol, 1.f));

    ss.str("");
    ss << (int)game_->getVictoryPoints(tid);
    FontManager::get()->drawString(ss.str(), pos, height);

    pos.x += size.x * 2.0;
  }

  // minimap underlay
  pos = convertUIPos(vec2Param("ui.minimap.pos"));
  size = vec2Param("ui.minimap.dim");
  tex = ResourceManager::get()->getTexture(strParam("ui.minimap.texture"));
  drawTexture(pos, size, tex);

  // unit info underlay:
  pos = convertUIPos(vec2Param("ui.unitinfo.pos"));
  size = vec2Param("ui.unitinfo.dim");
  tex = ResourceManager::get()->getTexture(strParam("ui.unitinfo.texture"));
  drawTexture(pos, size, tex);

  // minimap
  renderMinimap();

  // Render messages
  if (displayChatBoxTimer_ > 0.f ||
      player_->getState() == PlayerState::CHATTING) {
    pos = convertUIPos(vec2Param("hud.messages.pos"));
    float fontHeight = fltParam("hud.messages.fontHeight");
    float messageHeight = fontHeight + fltParam("hud.messages.heightBuffer");
    height = intParam("hud.messages.max") * messageHeight;
    glm::vec2 startPos = pos - glm::vec2(0, height);
    if (player_->getState() == PlayerState::CHATTING) height += messageHeight;
    size = glm::vec2(fltParam("hud.messages.backgroundWidth"), height);
    drawRect(startPos, size, vec4Param("hud.messages.backgroundColor"));
    const std::vector<std::string> &messages = player_->getChatMessages();
    int numMessages = std::min(intParam("hud.messages.max"),
        (int)messages.size());
    for (int i = 1; i <= numMessages; i++) {
      std::string message = messages[messages.size() - i];
      glm::vec2 messagePos = pos;
      messagePos.y -= i * messageHeight;
      FontManager::get()->drawString(message, messagePos, fontHeight);
    }
    std::string localMessage = player_->getLocalMessage();
    if (!localMessage.empty())
      FontManager::get()->drawString(localMessage, pos, fontHeight);
  }
  displayChatBoxTimer_ -= renderdt_;

  glEnable(GL_DEPTH_TEST);
}

void Renderer::renderMinimap() {
  const glm::vec2 &mapSize = game_->getMap()->getSize();
  const glm::vec4 &mapColor = game_->getMap()->getMinimapColor();
  // TODO(connor) we probably want some small buffer around the sides of the
  // minimap so we can see the underlay image..
  const glm::vec2 minimapSize = vec2Param("ui.minimap.dim");
  const glm::vec2 minimapPos = convertUIPos(vec2Param("ui.minimap.pos"));

  // TODO(connor) support other aspect ratios so they don't stretch or distort

  // Render base image
  drawRect(minimapPos, minimapSize, mapColor);

  // Render viewport
  glm::vec2 res = vec2Param("local.resolution");
  glm::vec2 minimapCoord[2][2];
  glm::vec4 lineColor = vec4Param("ui.minimap.viewportLineColor");

  // Iterates over the four corners of the current viewport
  for (int x = 0; x <= 1; x++) {
    for (int y = 0; y <= 1; y++) {
      glm::vec2 screenCoord(glm::vec2(x * res.x, y * res.y));
      glm::vec3 terrainCoord = screenToTerrain(screenCoord);
      if (terrainCoord.x == HUGE_VAL) terrainCoord.x = mapSize.x / 2;
      if (terrainCoord.x == -HUGE_VAL) terrainCoord.x = -mapSize.x / 2;
      if (terrainCoord.y == HUGE_VAL) terrainCoord.y = mapSize.y / 2;
      if (terrainCoord.y == -HUGE_VAL) terrainCoord.y = -mapSize.y / 2;
      minimapCoord[x][y] = worldToMinimap(terrainCoord);
    }
  }
  drawLine(minimapCoord[0][0], minimapCoord[0][1], lineColor);
  drawLine(minimapCoord[0][1], minimapCoord[1][1], lineColor);
  drawLine(minimapCoord[1][1], minimapCoord[1][0], lineColor);
  drawLine(minimapCoord[1][0], minimapCoord[0][0], lineColor);

  // render actors
  for (const auto &pair : mapCoords_) {
    const Entity *e = pair.first;
    const glm::vec3 &mapCoord = pair.second;
    glm::vec2 pos = worldToMinimap(mapCoord);
    const Player *player = game_->getPlayer(e->getPlayerID());
    glm::vec3 pcolor = player ? player->getColor() :
      vec3Param("global.defaultColor");
    // if selected draw as green
    glm::vec4 color = selection_.count(e->getID())
      ? glm::vec4(vec3Param("colors.selected"), 1.f)
      : glm::vec4(pcolor, 1.f);
    float actorSize = fltParam("ui.minimap.actorSize");
    drawRect(pos, glm::vec2(actorSize), color);
  }
}

void Renderer::renderMap(const Map *map) {
  const glm::vec2 &mapSize = map->getSize();

  const glm::vec4 &mapColor = map->getMinimapColor();

  GLuint mapProgram = ResourceManager::get()->getShader("map");
  glUseProgram(mapProgram);
  GLuint colorUniform = glGetUniformLocation(mapProgram, "color");
  GLuint mapSizeUniform = glGetUniformLocation(mapProgram, "mapSize");
  glUniform4fv(colorUniform, 1, glm::value_ptr(mapColor));
  glUniform2fv(mapSizeUniform, 1, glm::value_ptr(mapSize));

  glm::mat4 transform =
    glm::scale(
        glm::mat4(1.f),
        glm::vec3(mapSize.x, mapSize.y, 1.f));
  renderRectangleProgram(transform);

  // Render each of the highlights
  for (auto& hl : highlights_) {
    hl.remaining -= renderdt_;
    glm::mat4 transform =
      glm::scale(
          glm::translate(glm::mat4(1.f),
                         glm::vec3(hl.pos.x, hl.pos.y, 0.01f)),
        glm::vec3(0.33f));
    renderRectangleColor(transform, glm::vec4(1, 0, 0, 1));
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
}

void Renderer::startRender(float dt) {
  simdt_ = dt;
  if (lastRender_)
    renderdt_ = (SDL_GetTicks() - lastRender_) / 1000.f;
  else
    renderdt_ = 0;

  if (game_->isPaused()) {
    simdt_ = renderdt_ = 0.f;
  }

  glClearColor(0.f, 0.f, 0.f, 0.f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glEnable(GL_DEPTH_TEST);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  // Set up matrices
  float aspectRatio = resolution_.x / resolution_.y;
  float fov = 90.f;
  getProjectionStack().clear();
  getProjectionStack().current() =
    glm::perspective(fov, aspectRatio, 0.1f, 100.f);
  getViewStack().clear();
  getViewStack().current() =
    glm::lookAt(cameraPos_,
                glm::vec3(cameraPos_.x, cameraPos_.y + 0.5f, 0),
                glm::vec3(0, 1, 0));

  // Set up lights
  lightPos_ = applyMatrix(getViewStack().current(), glm::vec3(-5, -5, 10));

  // Display effects
  for (size_t i = 0; i < effects_.size(); i++) {
    Effect *effect = effects_[i];

    effect->render(renderdt_);

    if (effect->needsRemoval()) {
      delete effect;
      // Swap trick
      std::swap(effects_[i], effects_[effects_.size() - 1]);
      effects_.pop_back();
    }
  }

  // Clear coordinates
  ndcCoords_.clear();
  mapCoords_.clear();
  lastRender_ = SDL_GetTicks();
}

void Renderer::renderActor(const Actor *actor, glm::mat4 transform) {
  const Player *player = game_->getPlayer(actor->getPlayerID());
  glm::vec3 pcolor = player ? player->getColor() :
    vec3Param("global.defaultColor");
  // if selected draw as green
  glm::vec4 color = selection_.count(actor->getID())
                    ? glm::vec4(vec3Param("colors.selected"), 1.f)
                    : glm::vec4(pcolor, 1.f);
  // TODO(zack): Flash units white if damage taken
  const std::string &name = actor->getName();

  // TODO(zack) parameterize shaders on name
  GLuint meshProgram = ResourceManager::get()->getShader("unit");
  glUseProgram(meshProgram);
  GLuint colorUniform = glGetUniformLocation(meshProgram, "color");
  GLuint lightPosUniform = glGetUniformLocation(meshProgram, "lightPos");
  glUniform4fv(colorUniform, 1, glm::value_ptr(color));
  glUniform3fv(lightPosUniform, 1, glm::value_ptr(lightPos_));
  Mesh * mesh = ResourceManager::get()->getMesh(name);
  renderMesh(transform, mesh);

  glm::vec4 ndc = getProjectionStack().current() * getViewStack().current() *
                  transform * glm::vec4(0, 0, 0, 1);
  ndc /= ndc.w;
  ndcCoords_[actor] = glm::vec3(ndc);
  mapCoords_[actor] = applyMatrix(transform, glm::vec3(0.f));

  glDisable(GL_DEPTH_TEST);

  if (actor->getType() == Building::TYPE &&
      ((Building*)actor)->isCappable()) {
    Building *building = (Building*)actor;
    if (building->getCap() > 0.f &&
        building->getCap() < building->getMaxCap()) {
      // display health bar
      float capFact = glm::max(0.f, building->getCap() / building->getMaxCap());
      glm::vec2 size = vec2Param("hud.actor_cap.dim");
      glm::vec2 offset = vec2Param("hud.actor_cap.pos");
      glm::vec2 pos = (glm::vec2(ndc.x, -ndc.y) / 2.f + glm::vec2(0.5f)) *
        resolution_;
      pos += offset;
      // Black underneath
      drawRectCenter(pos, size, glm::vec4(0, 0, 0, 1));
      pos.x -= size.x * (1.f - capFact) / 2.f;
      size.x *= capFact;
      // Get player color: If it is owned by a player, use that color.
      // If it it unowned, used the capping player's color.
      glm::vec3 cap_color = pcolor;
      if (building->getPlayerID() == NO_PLAYER) {
        id_t pid = building->getLastCappingPlayerID();
        cap_color = game_->getPlayer(pid)->getColor();
      }
      drawRectCenter(pos, size, glm::vec4(cap_color, 1));
    }
  } else {
    // display health bar
    float healthFact = glm::max(
        0.f,
        actor->getHealth() / actor->getMaxHealth());
    glm::vec2 size = vec2Param("hud.actor_health.dim");
    glm::vec2 offset = vec2Param("hud.actor_health.pos");
    glm::vec2 pos = (glm::vec2(ndc.x, -ndc.y) / 2.f + glm::vec2(0.5f)) *
      resolution_;
    pos += offset;
    // Red underneath for max health
    drawRectCenter(pos, size, glm::vec4(1, 0, 0, 1));
    // Green on top for current health
    pos.x -= size.x * (1.f - healthFact) / 2.f;
    size.x *= healthFact;
    drawRectCenter(pos, size, glm::vec4(0, 1, 0, 1));
  }

  std::queue<Actor::Production> queue = actor->getProductionQueue();
  if (!queue.empty()) {
    // display production bar
    float prodFactor = 1.f - queue.front().time / queue.front().max_time;
    glm::vec2 size = vec2Param("hud.actor_prod.dim");
    glm::vec2 offset = vec2Param("hud.actor_prod.pos");
    glm::vec2 pos = (glm::vec2(ndc.x, -ndc.y) / 2.f + glm::vec2(0.5f))
        * resolution_;
    pos += offset;
    // Purple underneath for max time
    drawRectCenter(pos, size, glm::vec4(0.5, 0, 1, 1));
    // Blue on top for time elapsed
    pos.x -= size.x * (1.f - prodFactor) / 2.f;
    size.x *= prodFactor;
    drawRectCenter(pos, size, glm::vec4(0, 0, 1, 1));
  }

  glEnable(GL_DEPTH_TEST);
}

void Renderer::endRender() {
  glm::mat4 fullmat =
    getProjectionStack().current() * getViewStack().current();
  // Render drag selection if it exists
  if (dragStart_ != glm::vec3(HUGE_VAL)) {
    glm::vec2 start = applyMatrix(fullmat, dragStart_).xy;
    glm::vec2 end = applyMatrix(fullmat, dragEnd_).xy;
    start = (glm::vec2(start.x, -start.y) + glm::vec2(1.f))
        * 0.5f * resolution_;
    end = (glm::vec2(end.x, -end.y) + glm::vec2(1.f))
        * 0.5f * resolution_;

    glDisable(GL_DEPTH_TEST);

    // TODO(zack): make this color a param
    drawRectCenter((start + end) / 2.f, end - start,
        glm::vec4(0.2f, 0.6f, 0.2f, 0.3f));
    // Reset each frame
    dragStart_ = glm::vec3(HUGE_VAL);
  }

  renderUI();

  SDL_GL_SwapBuffers();
}

void Renderer::updateCamera(const glm::vec3 &delta) {
  cameraPos_ += delta;

  glm::vec2 mapSize = game_->getMap()->getSize() / 2.f;
  cameraPos_ = glm::clamp(
                 cameraPos_,
                 glm::vec3(-mapSize.x, -mapSize.y, 0.f),
                 glm::vec3(mapSize.x, mapSize.y, 20.f));
}

void Renderer::minimapUpdateCamera(const glm::vec2 &screenCoord) {
  glm::vec2 minimapPos = convertUIPos(vec2Param("ui.minimap.pos"));
  glm::vec2 minimapDim = vec2Param("ui.minimap.dim");
  glm::vec2 mapSize = game_->getMap()->getSize();
  glm::vec2 mapCoord = screenCoord;
  mapCoord -= minimapPos;
  mapCoord -= glm::vec2(minimapDim.x / 2, minimapDim.y / 2);
  mapCoord *= mapSize / minimapDim;
  mapCoord.y *= -1;
  glm::vec3 camCoord = cameraPos_;
  glm::vec3 cameraDelta =
      glm::vec3(mapCoord.x - camCoord.x, mapCoord.y - camCoord.y, 0);
  updateCamera(cameraDelta);
}

id_t Renderer::selectEntity(const glm::vec2 &screenCoord) const {
  glm::vec2 pos = glm::vec2(screenToNDC(screenCoord));

  id_t eid = NO_ENTITY;
  float bestDist = HUGE_VAL;
  // Find the best entity
  for (const auto& pair : ndcCoords_) {
    float dist = glm::distance(pos, glm::vec2(pair.second));
    // TODO(zack) transform radius and use it instead of hardcoded number..
    // Must be inside the entities radius and better than previous
    const float thresh = sqrtf(0.009f);
    if (dist < thresh && dist < bestDist) {
      bestDist = dist;
      eid = pair.first->getID();
    }
  }

  return eid;
}

std::set<id_t> Renderer::selectEntities(
  const glm::vec3 &start, const glm::vec3 &end, id_t pid) const {
  assertPid(pid);
  glm::mat4 fullMatrix =
    getProjectionStack().current() * getViewStack().current();
  glm::vec2 s = applyMatrix(fullMatrix, start).xy;
  glm::vec2 e = applyMatrix(fullMatrix, end).xy;
  // Defines the rectangle
  glm::vec2 center = (e + s) / 2.f;
  glm::vec2 size = glm::abs(e - s);
  std::set<id_t> ret;

  for (const auto &pair : ndcCoords_) {
    glm::vec2 p = glm::vec2(pair.second);
    // Inside rect and owned by player
    // TODO(zack) make this radius aware, right now the center must be in
    // their.
    if (fabs(p.x - center.x) < size.x / 2.f
        && fabs(p.y - center.y) < size.y / 2.f
        && pair.first->getPlayerID() == pid) {
      ret.insert(pair.first->getID());
    }
  }

  return ret;
}

void Renderer::setSelection(const std::set<id_t> &select) {
  selection_ = select;
}

void Renderer::highlight(const glm::vec2 &mapCoord) {
  MapHighlight hl;
  hl.pos = mapCoord;
  hl.remaining = 0.5f;
  highlights_.push_back(hl);
}

void Renderer::setDragRect(const glm::vec3 &s, const glm::vec3 &e) {
  dragStart_ = s;
  dragEnd_ = e;
}

glm::vec3 Renderer::screenToTerrain(const glm::vec2 &screenCoord) const {
  glm::vec4 ndc = glm::vec4(screenToNDC(screenCoord), 1.f);

  ndc = glm::inverse(
      getProjectionStack().current() * getViewStack().current()) * ndc;
  ndc /= ndc.w;

  glm::vec3 dir = glm::normalize(glm::vec3(ndc) - cameraPos_);

  glm::vec3 terrain = glm::vec3(ndc) - dir * ndc.z / dir.z;

  const glm::vec2 mapSize = game_->getMap()->getSize() / 2.f;
  // Don't return a non terrain coordinate
  if (terrain.x < -mapSize.x) {
    terrain.x = -HUGE_VAL;
  } else if (terrain.x > mapSize.x) {
     terrain.x = HUGE_VAL;
  }
  if (terrain.y < -mapSize.y) {
      terrain.y = -HUGE_VAL;
  } else if (terrain.y > mapSize.y) {
      terrain.y = HUGE_VAL;
  }

  return glm::vec3(terrain);
}

glm::vec3 Renderer::screenToNDC(const glm::vec2 &screen) const {
  float z;
  glReadPixels(screen.x, resolution_.y - screen.y, 1, 1,
      GL_DEPTH_COMPONENT, GL_FLOAT, &z);
  return glm::vec3(
      2.f * (screen.x / resolution_.x) - 1.f,
      2.f * (1.f - (screen.y / resolution_.y)) - 1.f,
      z);
}

BloodEffect::BloodEffect(id_t aid)
  : aid_(aid),
    t_(0.f) {
  assertEid(aid);
}

BloodEffect::~BloodEffect() {
  // nop
}

void BloodEffect::render(float dt) {
  // TODO(zack) replace this with a cool blood effect and or damage text
  t_ += dt;

  Actor *a = (Actor *) Game::get()->getEntity(aid_);
  // TODO(zack): assert that this entity IS an actor
  if (!a) {
    // If entity died, no more effect
    t_ = HUGE_VAL;
    return;
  }

  glm::vec3 pos = glm::vec3(a->getPosition(), 0)
    + glm::vec3(0.f, 0.f, a->getHeight());
  float modelSize = fltParam(a->getName() + ".modelSize");
  float size = modelSize / 5.0f;
  glm::mat4 transform =
      glm::rotate(
        glm::scale(
          glm::translate(glm::mat4(1.f), pos),
          glm::vec3(size)),
        rad2deg(M_PI * t_),
        glm::vec3(0, 0, 1));

  renderRectangleColor(transform, glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
}

bool BloodEffect::needsRemoval() const {
  return t_ >= fltParam("effects.damageRenderDuration");
}
};  // rts
