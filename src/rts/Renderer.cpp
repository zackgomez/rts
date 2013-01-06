#define GLM_SWIZZLE_XYZW
#include "rts/Renderer.h"
#include <sstream>
#include <SDL/SDL.h>
#include <GL/glew.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/norm.hpp>
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
}

Renderer::~Renderer() {
  delete controller_;
  delete ui_;
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
  Clock::time_point last = Clock::now();
  while (running_) {
    if (controller_) {
      controller_->processInput(fps);
    }

    render();
    Clock::dumpTimes();

    // Regulate frame rate
    float delay = glm::clamp(2 * fps - Clock::secondsSince(last), 0.f, fps);
    last = Clock::now();
    std::chrono::milliseconds delayms(static_cast<int>(1000 * delay));
    std::this_thread::sleep_for(delayms);
  }
}

void Renderer::render() {
  Clock::startSection("render");
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
}

void Renderer::renderUI() {
  record_section("renderUI");
  if (ui_) {
    ui_->render(renderdt_);
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

  lastRender_ = SDL_GetTicks();
}

void Renderer::endRender() {
  renderUI();

  SDL_GL_SwapBuffers();
}

void Renderer::updateCamera(const glm::vec3 &delta) {
  setCameraPos(cameraPos_ + delta);
}

void Renderer::setCameraPos(const glm::vec3 &pos) {
  auto mapExtent = mapSize_ / 2.f;
  cameraPos_ = glm::clamp(
      pos,
      glm::vec3(-mapExtent.x, -mapExtent.y, 0.f),
      glm::vec3(mapExtent.x, mapExtent.y, 20.f));
}

glm::vec3 Renderer::screenToTerrain(const glm::vec2 &screenCoord) const {
  glm::vec4 ndc = glm::vec4(screenToNDC(screenCoord), 1.f);

  ndc = glm::inverse(
      getProjectionStack().current() * getViewStack().current()) * ndc;
  ndc /= ndc.w;

  glm::vec3 dir = glm::normalize(glm::vec3(ndc) - cameraPos_);

  glm::vec3 terrain = glm::vec3(ndc) - dir * ndc.z / dir.z;

  auto mapExtent = mapSize_ / 2.f;
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
