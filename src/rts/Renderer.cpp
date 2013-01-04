#define GLM_SWIZZLE_XYZW
#include "rts/Renderer.h"
#include <sstream>
#include <SDL/SDL.h>
#include <GL/glew.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>
#include "common/Clock.h"
#include "common/Collision.h"
#include "common/ParamReader.h"
#include "common/util.h"
#include "rts/Building.h"
#include "rts/CollisionObject.h"
#include "rts/Controller.h"
#include "rts/Graphics.h"
#include "rts/GameEntity.h"
#include "rts/FontManager.h"
#include "rts/Game.h"
#include "rts/Map.h"
#include "rts/MessageHub.h"
#include "rts/Projectile.h"
#include "rts/Player.h"
#include "rts/ModelEntity.h"
#include "rts/ResourceManager.h"
#include "rts/Unit.h"
#include "rts/UI.h"

namespace rts {

id_t Entity::lastID_ = STARTING_EID;

Renderer::Renderer()
  : controller_(nullptr),
    running_(true),
    cameraPos_(0.f, 0.f, 5.f),
    resolution_(vec2Param("local.resolution")),
    timeMultiplier_(1.f),
    lastTickTime_(Clock::now()),
    lastRender_(0) {
  // TODO(zack): move this to a separate initialize function
  initEngine(resolution_);
  // Initialize font manager, if necessary
  FontManager::get();

  // TODO(zack): move to ResourceManager
  // unit model is based at 0, height 1, translate to center of model
  glm::mat4 unitMeshTrans = glm::scale(glm::mat4(1.f), glm::vec3(1, 0.5f, 1));
  setMeshTransform(ResourceManager::get()->getMesh("unit"), unitMeshTrans);
  setMeshTransform(
    ResourceManager::get()->getMesh("melee_unit"),
    glm::rotate(
      glm::scale(unitMeshTrans, glm::vec3(2.f)),
      90.f, glm::vec3(0, 1, 0)));

  glm::mat4 projMeshTrans =
    glm::rotate(glm::mat4(1.f), 90.f, glm::vec3(1, 0, 0));
  setMeshTransform(
    ResourceManager::get()->getMesh("basic_bullet"),
    projMeshTrans);
}

Renderer::~Renderer() {
  delete controller_;
  delete ui_;
}

Renderer *Renderer::get() {
  static Renderer instance;
  return &instance;
}

void Renderer::setController(Controller *controller) {
  if (controller_) {
    delete controller_;
  }
  controller_ = controller;
}

void Renderer::setUI(UI *ui) {
  if (ui_) {
    delete ui_;
  }
  ui_ = ui;
}

void Renderer::startMainloop() {
  Clock::setThread();
  const float framerate = fltParam("local.framerate");
  float fps = 1.f / framerate;

  // render loop
  while (running_) {
    uint32_t start = SDL_GetTicks();
    if (controller_) {
      controller_->processInput(fps);
    }
    Renderer::get()->getController()->processInput(fps);
    Clock::startSection("render");
    render();
    Clock::endSection("render");
    Clock::dumpTimes();

    // Regulate frame rate
    uint32_t end = SDL_GetTicks();
    uint32_t delay = std::max(int(1000*fps) - int(end - start), 0);
    SDL_Delay(delay);
  }
}

void Renderer::addChatMessage(const std::string &message) {
  chats_.emplace_back(message, Clock::now());
}

void Renderer::render() {
  // lock
  std::unique_lock<std::mutex> lock(mutex_);

  simdt_ = Clock::secondsSince(lastTickTime_);

  startRender();

  renderMap();

  for (auto &it : Game::get()->getEntities()) {
    renderEntity(it.second);
  }

  endRender();

  // unlock when lock goes out of scope
}

void Renderer::renderEntity(ModelEntity *entity) {
  record_section("renderEntity");
  if (!entity->hasProperty(ModelEntity::P_RENDERABLE)) {
      return;
  }
  entity->render(simdt_);
  auto transform = entity->getTransform(simdt_);
  if (ui_) {
    ui_->renderEntity(entity, transform, renderdt_);
  }

  glm::vec4 ndc = getProjectionStack().current() * getViewStack().current() *
                  transform * glm::vec4(0, 0, 0, 1);
  ndc /= ndc.w;
  ndcCoords_[entity] = glm::vec3(ndc);
}

glm::vec2 Renderer::convertUIPos(const glm::vec2 &pos) const {
  return glm::vec2(
      pos.x < 0 ? pos.x + resolution_.x : pos.x,
      pos.y < 0 ? pos.y + resolution_.y : pos.y);
}

void Renderer::renderUI() {
  record_section("renderUI");
  glDisable(GL_DEPTH_TEST);

  // Actor info health/mana etc for actors on the screen
  renderActorInfo();

  if (ui_) {
    ui_->render(renderdt_);
  }

  glEnable(GL_DEPTH_TEST);
}

void Renderer::renderActorInfo() {
  record_section("renderActor");
  for (auto pair : ndcCoords_) {
    const GameEntity *e = (const GameEntity *)pair.first;
    auto &ndc = pair.second;
    auto type = e->getType();
    // TODO(zack): only render status for actors currently on screen/visible to
    // player
    if (!e->hasProperty(GameEntity::P_ACTOR)) {
      continue;
    }
    auto actor = (const Actor *)e;
    if (actor->hasProperty(GameEntity::P_CAPPABLE)) {
        Building *building = (Building*)actor;
        if (building->getCap() > 0.f &&
          building->getCap() < building->getMaxCap()) {
            // display health bar
            float capFact = glm::max(
                building->getCap() / building->getMaxCap(),
                0.f);
            glm::vec2 size = vec2Param("hud.actor_cap.dim");
            glm::vec2 offset = vec2Param("hud.actor_cap.pos");
            glm::vec2 pos = (glm::vec2(ndc.x, -ndc.y) / 2.f + glm::vec2(0.5f)) *
              resolution_;
            pos += offset;
            // Black underneath
            drawRectCenter(pos, size, glm::vec4(0, 0, 0, 1));
            pos.x -= size.x * (1.f - capFact) / 2.f;
            size.x *= capFact;
            const glm::vec4 cap_color = vec4Param("hud.actor_cap.color");
            drawRectCenter(pos, size, cap_color);
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

    auto queue = actor->getProductionQueue();
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
  }
}

void Renderer::renderMap() {
  record_section("renderMap");

  auto gridDim = mapSize_ * 2.f;

  GLuint mapProgram = ResourceManager::get()->getShader("map");
  glUseProgram(mapProgram);
  GLuint colorUniform = glGetUniformLocation(mapProgram, "color");
  GLuint mapSizeUniform = glGetUniformLocation(mapProgram, "mapSize");
  GLuint gridDimUniform = glGetUniformLocation(mapProgram, "gridDim");
  glUniform4fv(colorUniform, 1, glm::value_ptr(mapColor_));
  glUniform2fv(mapSizeUniform, 1, glm::value_ptr(mapSize_));
  glUniform2fv(gridDimUniform, 1, glm::value_ptr(gridDim));

  // TODO(zack): render map with height/terrain map
  glm::mat4 transform =
    glm::scale(
        glm::mat4(1.f),
        glm::vec3(mapSize_.x, mapSize_.y, 1.f));
  renderRectangleProgram(transform);
}

void Renderer::startRender() {
  if (lastRender_)
    renderdt_ = (SDL_GetTicks() - lastRender_) / 1000.f;
  else
    renderdt_ = 0;

  simdt_ *= timeMultiplier_;
  renderdt_ *= timeMultiplier_;

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
  // TODO(zack): read light pos from map config
  auto lightPos = applyMatrix(getViewStack().current(), glm::vec3(-5, -5, 10));
  setParam("renderer.lightPos", lightPos);

  // Clear coordinates
  ndcCoords_.clear();
  lastRender_ = SDL_GetTicks();
}

void Renderer::endRender() {
  renderUI();

  SDL_GL_SwapBuffers();
}

void Renderer::updateCamera(const glm::vec3 &delta) {
  cameraPos_ += delta;

  auto mapExtent = mapSize_ / 2.f;
  cameraPos_ = glm::clamp(
      cameraPos_,
      glm::vec3(-mapExtent.x, -mapExtent.y, 0.f),
      glm::vec3(mapExtent.x, mapExtent.y, 20.f));
}

void Renderer::minimapUpdateCamera(const glm::vec2 &screenCoord) {
  const glm::vec2 minimapPos = convertUIPos(vec2Param("ui.minimap.pos"));
  const glm::vec2 minimapDim = vec2Param("ui.minimap.dim");
  glm::vec2 mapCoord = screenCoord;
  mapCoord -= minimapPos;
  mapCoord -= glm::vec2(minimapDim.x / 2, minimapDim.y / 2);
  mapCoord *= mapSize_ / minimapDim;
  mapCoord.y *= -1;
  const glm::vec3 camCoord = cameraPos_;
  const glm::vec3 cameraDelta =
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
      eid = ((GameEntity *)pair.first)->getID();
    }
  }

  return eid;
}

std::set<id_t> Renderer::selectEntities(
  const glm::vec3 &start, const glm::vec3 &end, id_t pid) const {
  assertPid(pid);
  std::set<id_t> ret;
  glm::mat4 fullMatrix =
    getProjectionStack().current() * getViewStack().current();
  glm::vec2 s = applyMatrix(fullMatrix, start).xy;
  glm::vec2 e = applyMatrix(fullMatrix, end).xy;
  // Defines the rectangle
  glm::vec2 center = (e + s) / 2.f;
  glm::vec2 size = glm::abs(e - s);

  for (const auto &pair : ndcCoords_) {
    glm::vec2 p = glm::vec2(pair.second);
    const GameEntity *e = (GameEntity *)pair.first;
    // Inside rect and owned by player
    // TODO(zack) make this radius aware, right now the center must be in
    // their.
    if (fabs(p.x - center.x) < size.x / 2.f
        && fabs(p.y - center.y) < size.y / 2.f
        && e->getPlayerID() == pid) {
      ret.insert(e->getID());
    }
  }

  return ret;
}

glm::vec3 Renderer::screenToTerrain(const glm::vec2 &screenCoord) const {
  glm::vec4 ndc = glm::vec4(screenToNDC(screenCoord), 1.f);

  ndc = glm::inverse(
      getProjectionStack().current() * getViewStack().current()) * ndc;
  ndc /= ndc.w;

  glm::vec3 dir = glm::normalize(glm::vec3(ndc) - cameraPos_);

  glm::vec3 terrain = glm::vec3(ndc) - dir * ndc.z / dir.z;

  const glm::vec2 mapExtent = mapSize_ / 2.f;
  // Don't return a non terrain coordinate
  if (terrain.x < -mapExtent.x) {
    terrain.x = -HUGE_VAL;
  } else if (terrain.x > mapExtent.x) {
     terrain.x = HUGE_VAL;
  }
  if (terrain.y < -mapExtent.y) {
      terrain.y = -HUGE_VAL;
  } else if (terrain.y > mapExtent.y) {
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
};  // rts
