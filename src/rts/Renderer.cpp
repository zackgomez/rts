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
    camera_(glm::vec3(0.f), 5.f, 0.f, 45.f),
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
}

void Renderer::setController(Controller *controller) {
  if (controller_) {
    delete controller_;
  }
  controller_ = controller;
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

  for (auto &it : entities_) {
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
  UI::get()->renderEntity(entity, transform, renderdt_);
}

void Renderer::renderUI() {
  record_section("renderUI");
  UI::get()->render(renderdt_);
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
  float aspect = resolution_.x / resolution_.y;
  float fov = 90.f;
  getProjectionStack().clear();
  getProjectionStack().current() = glm::perspective(fov, aspect, 0.1f, 100.f);
  getViewStack().clear();
  getViewStack().current() = camera_.calculateViewMatrix();

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

void Renderer::spawnEntity(Entity *ent) {
  invariant(ent, "Cannot spawn null entity");
  invariant(entities_.find(ent->getID()) == entities_.end(),
      "cannot add spawn entity with already existing ID");
  entities_[ent->getID()] = (GameEntity *)ent;
}

void Renderer::removeEntity(id_t eid) {
  auto *e = entities_[eid];
  entities_.erase(eid);
  delete e;
}

void Renderer::updateCamera(const glm::vec3 &delta) {
  glm::mat4 viewMat = glm::inverse(camera_.calculateViewMatrix());

  glm::vec3 forward = applyMatrix(viewMat, glm::vec3(0, 1, 0), false);
  forward = glm::normalize(glm::vec3(forward.x, forward.y, 0));

  glm::vec3 right = applyMatrix(viewMat, glm::vec3(1, 0, 0), false);
  right = glm::normalize(glm::vec3(right.x, right.y, 0));

  auto transformedDelta = delta.x * right + delta.y * forward;
  setCameraLookAt(camera_.getLookAt() + transformedDelta);
}

void Renderer::rotateCamera(const glm::vec2 &rot) {
  camera_.setTheta(camera_.getTheta() + rot.x);
  camera_.setPhi(camera_.getPhi() + rot.y);
}
void Renderer::resetCameraRotation() {
  camera_.setTheta(0.f);
  camera_.setPhi(45.f);
}

void Renderer::zoomCamera(float delta) {
  camera_.setZoom(glm::clamp(camera_.getZoom() + delta, 1.5f, 10.f));
}

void Renderer::setCameraLookAt(const glm::vec3 &pos) {
  auto mapExtent = mapSize_ / 2.f;
  camera_.setLookAt(glm::clamp(
      pos,
      glm::vec3(-mapExtent.x, -mapExtent.y, 0.f),
      glm::vec3(mapExtent.x, mapExtent.y, 20.f)));
}

glm::vec3 Renderer::screenToTerrain(const glm::vec2 &screenCoord) const {
  glm::vec3 ndc = screenToNDC(screenCoord);

  auto inverseProjMat = glm::inverse(getProjectionStack().current());
  auto inverseViewMat = glm::inverse(getViewStack().current());
  glm::vec3 cameraDir = glm::normalize(
      applyMatrix(inverseProjMat, glm::vec3(ndc)));
  glm::vec3 worldDir = glm::normalize(
      applyMatrix(inverseViewMat, cameraDir, false));
  glm::vec3 worldPos = applyMatrix(inverseViewMat, glm::vec3(0.f));

  // for now, just project into the ground
  float t = (0.f - worldPos.z) / worldDir.z;
  auto terrain = worldPos + t * worldDir;

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
  return glm::vec3(
      (2.f * (screen.x / resolution_.x) - 1.f),
      2.f * (1.f - (screen.y / resolution_.y)) - 1.f,
      1.f);
}
};  // rts
