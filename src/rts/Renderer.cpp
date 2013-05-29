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
#include "common/FPSCalculator.h"
#include "common/ParamReader.h"
#include "common/util.h"
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

Renderer::Renderer()
  : controller_(nullptr),
    running_(true),
    camera_(glm::vec3(0.f), 5.f, 0.f, 45.f),
    resolution_(vec2Param("local.resolution")),
    timeMultiplier_(1.f),
    lastTickTime_(Clock::now()),
    lastRender_(Clock::now()),
    mapSize_(0.f),
    nextEntityID_(STARTING_EID) {
  // TODO(zack): move this to a separate initialize function
  initEngine(resolution_);

#ifdef USE_FMOD
  FMOD_RESULT result;
  FMOD::System *system_;

  result = FMOD::System_Create(&system_);		// Create the main system object.
  if (result != FMOD_OK)
  {
    printf("FMOD error! (%d)\n", result);
    exit(-1);
  }

  result = system_->init(50, FMOD_INIT_NORMAL, 0);	// Initialize FMOD.
  if (result != FMOD_OK)
  {
    printf("FMOD error! (%d)\n", result);
    exit(-1);
  }
#endif  // USE_FMOD
  // Initialize font manager, if necessary
  FontManager::get();
}

Renderer::~Renderer() {
  clearController();
  clearEntities();
}

void Renderer::postToMainThread(const PostableFunction &func) {
  std::unique_lock<std::mutex> lock(funcMutex_);
  funcQueue_.push_back(func);
}

void Renderer::clearEntities() {
  for (auto&& pair : entities_) {
    delete pair.second;
  }
  entities_.clear();
  nextEntityID_ = STARTING_EID;
}

void Renderer::setController(Controller *controller) {
  clearController();
  controller_ = controller;
  if (controller_) {
    controller_->onCreate();
  }
}

void Renderer::clearController() {
  if (controller_) {
    controller_->onDestroy();
    delete controller_;
  }
  controller_ = nullptr;
}

void Renderer::startMainloop() {
  Clock::setThread();
  const float framerate = fltParam("local.framerate");
  float fps = 1.f / framerate;

  FPSCalculator updateTimer(64);
  // render loop
  Clock::time_point last = Clock::now();
  while (running_) {
    std::unique_lock<std::mutex> lock(mutex_);

    // Run 'posted' functions
    std::unique_lock<std::mutex> fqueueLock(funcMutex_);
    for (auto&& func : funcQueue_) {
      func();
    }
    funcQueue_.clear();
    fqueueLock.unlock();

    if (controller_) {
      controller_->processInput(fps);
    }

    render();
    lock.unlock();
    Clock::dumpTimes();

    // Regulate frame rate
    float delay = glm::clamp(2 * fps - Clock::secondsSince(last), 0.f, fps);
    last = Clock::now();
    std::chrono::milliseconds delayms(static_cast<int>(1000 * delay));
    std::this_thread::sleep_for(delayms);
    averageFPS_ = updateTimer.sample();
  }
}

void Renderer::render() {
  Clock::startSection("render");

  simdt_ = Clock::secondsSince(lastTickTime_);

  startRender();

  renderMap();

  for (auto &it : entities_) {
    renderEntity(it.second);
  }
  if (entityOverlayRenderer_) {
    for (auto &it : entities_) {
      renderEntityOverlay(it.second);
    }
  }

  endRender();
}

void Renderer::renderEntity(ModelEntity *entity) {
  record_section("renderEntity");
  if (!entity->hasProperty(ModelEntity::P_RENDERABLE)) {
    return;
  }
  if (!entity->isVisible()) {
    return;
  }
  entity->render(simdt_);
}

void Renderer::renderEntityOverlay(ModelEntity *entity) {
  record_section("renderEntity");
  if (!entity->hasProperty(ModelEntity::P_RENDERABLE)) {
      return;
  }
	if (!entity->isVisible()) {
			return;
	}
  entityOverlayRenderer_(entity, simdt_);
}

void Renderer::renderUI() {
  record_section("renderUI");
  if (controller_) {
    controller_->render(renderdt_);
  }
}

void Renderer::renderMap() {
  record_section("renderMap");
  if (mapSize_ == glm::vec2(0.f)) {
    return;
  }
  auto gridDim = mapSize_ * 2.f;

  auto mapShader = ResourceManager::get()->getShader("map");
  mapShader->makeActive();
  mapShader->uniform4f("color", mapColor_);
  mapShader->uniform2f("mapSize", mapSize_);
  mapShader->uniform2f("gridDim", gridDim);

  // TODO(zack): HACK ALERT this is for fog of war, render as a separate
  // step instead
	if (controller_) {
		controller_->updateMapShader(mapShader);
	}

  // TODO(zack): render map with height/terrain map
  glm::mat4 transform =
    glm::scale(
        glm::mat4(1.f),
        glm::vec3(mapSize_.x, mapSize_.y, 1.f));
  renderRectangleProgram(transform);
}

void Renderer::startRender() {
  renderdt_ = Clock::secondsSince(lastRender_);

  simdt_ *= timeMultiplier_;
  renderdt_ *= timeMultiplier_;

  glClearColor(0.f, 0.f, 0.f, 0.f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glEnable(GL_DEPTH_TEST);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  // Set up matrices
  float aspect = resolution_.x / resolution_.y;
  float fov = 60.f;
  getProjectionStack().clear();
  getProjectionStack().current() = glm::perspective(fov, aspect, 0.1f, 100.f);
  getViewStack().clear();
  getViewStack().current() = camera_.calculateViewMatrix();

  // Set up lights
  // TODO(zack): read light pos from map config
  auto lightPos = applyMatrix(getViewStack().current(), glm::vec3(-5, -5, 10));
  setParam("renderer.lightPos", lightPos);
  setParam("renderer.light.ambient", glm::vec3(0.1f));
  setParam("renderer.light.diffuse", glm::vec3(1.f));
  setParam("renderer.light.specular", glm::vec3(1.f));

  lastRender_ = Clock::now();
}

void Renderer::endRender() {
  renderUI();

  SDL_GL_SwapBuffers();
}

const GameEntity *Renderer::castRay(
    const glm::vec3 &origin,
    const glm::vec3 &dir,
    std::function<bool(const GameEntity *)> filter) const {
  float bestTime = HUGE_VAL;
  const GameEntity *ret = nullptr;
  for (auto pair : entities_) {
    auto entity = pair.second;
    float time = rayAABBIntersection(
      origin,
      dir,
      entity->getPosition() + glm::vec3(0.f, 0.f, entity->getHeight() / 2.f),
      glm::vec3(entity->getSize(), entity->getHeight()));
    if (time != NO_INTERSECTION) {
      if (time < bestTime && (!filter || filter(entity))) {
        bestTime = time;
        ret = entity;
      }
    }
  }

  return ret;
}

void Renderer::getNearbyEntities(
    const glm::vec3 &pos,
    float radius,
    std::function<bool(const GameEntity *)> callback) const {
  float radius2 = radius * radius;
  for (auto pair : entities_) {
    auto entity = pair.second;
    glm::vec3 diff = entity->getPosition() - pos;

    float dist2 = glm::dot(diff, diff);
    if (dist2 < radius2) {
      if (!callback(entity)) {
        return;
      }
    }
  }
}
std::vector<const GameEntity *> Renderer::getNearbyEntitiesArray(
    const glm::vec3& pos,
    float radius) {
  std::vector<const GameEntity *> ret;
  getNearbyEntities(
      pos,
      radius,
      [&ret] (const GameEntity *e) -> bool {
        ret.push_back(e);
        return true;
      });
  return ret;
}

id_t Renderer::newEntityID() {
  // this is an atomic variable, safe!
  return nextEntityID_++;
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
  camera_.setPhi(55.f);
  camera_.setZoom(15.f);
}

void Renderer::zoomCamera(float delta) {
  camera_.setZoom(glm::clamp(camera_.getZoom() + delta, 1.5f, 25.f));
}

void Renderer::setCameraLookAt(const glm::vec3 &pos) {
  auto mapExtent = mapSize_ / 2.f;
  camera_.setLookAt(glm::clamp(
      pos,
      glm::vec3(-mapExtent.x, -mapExtent.y, 0.f),
      glm::vec3(mapExtent.x, mapExtent.y, 20.f)));
}

std::tuple<glm::vec3, glm::vec3> Renderer::screenToRay(
    const glm::vec2 &screenCoord) const {
  glm::vec3 ndc = screenToNDC(screenCoord);
  auto inverseProjMat = glm::inverse(getProjectionStack().current());
  auto inverseViewMat = glm::inverse(getViewStack().current());
  glm::vec3 cameraDir = glm::normalize(
      applyMatrix(inverseProjMat, glm::vec3(ndc)));
  glm::vec3 worldDir = glm::normalize(
      applyMatrix(inverseViewMat, cameraDir, false));
  glm::vec3 worldPos = applyMatrix(inverseViewMat, glm::vec3(0.f));

  return std::make_tuple(worldPos, worldDir);
}

glm::vec3 Renderer::screenToTerrain(const glm::vec2 &screenCoord) const {
  glm::vec3 worldPos, worldDir;
  std::tie(worldPos, worldDir) = screenToRay(screenCoord);

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
