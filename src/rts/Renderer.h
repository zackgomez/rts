#ifndef SRC_RTS_RENDERER_H_
#define SRC_RTS_RENDERER_H_
#include <atomic>
#include <map>
#include <mutex>
#include <set>
#include <GL/glew.h>
#include <glm/glm.hpp>
#include "common/Clock.h"
#include "common/Logger.h"
#include "rts/Camera.h"
#include "rts/GameEntity.h"
#ifdef USE_FMOD
#include <fmod.hpp>
#endif  // USE_FMOD

namespace rts {

class Actor;
class Controller;
class Game;
class Map;
class ModelEntity;
class UI;

struct MapHighlight;

class Renderer {
 public:
  ~Renderer();

  static Renderer* get() {
    static Renderer instance;
    return &instance;
  }

  float getSimDT() const {
    return simdt_;
  }
  float getAverageFPS() const {
    return averageFPS_;
  }
  const glm::vec2& getResolution() const {
    return resolution_;
  }
  const glm::vec3& getCameraPos() const {
    return camera_.getLookAt();
  }
  const glm::vec2& getMapSize() const {
    return mapSize_;
  }
  Controller *getController() const {
    return controller_;
  }

  std::map<id_t, GameEntity *>& getEntities() {
    return entities_;
  }
  const std::map<id_t, GameEntity *>& getEntities() const {
    return entities_;
  }
  // Internally synchronized
  id_t newEntityID();
  void spawnEntity(Entity *ent);
  void removeEntity(id_t eid);
  void clearEntities();

  // Sets the last update time for inter/extrapolation purposes
  void setLastTickTime(const Clock::time_point &t) {
    lastTickTime_ = t;
  }
  void setTimeMultiplier(float t) {
    timeMultiplier_ = t;
  }
  // eventually replace this with a set map geometry or something similar
  void setMapSize(const glm::vec2 &mapSize) {
    mapSize_ = mapSize;
  }
  void setMapColor(const glm::vec4 &mapColor) {
    mapColor_ = mapColor;
  }
  void setCameraLookAt(const glm::vec3 &pos);
  // @param rot.x theta rotation in degrees
  // @param rot.y phi rotation in degrees
  void rotateCamera(const glm::vec2 &rot);
  void resetCameraRotation();
  void zoomCamera(float delta);

  void setController(Controller *controller);
  void clearController();

  void startMainloop();
  void updateCamera(const glm::vec3 &delta);
  std::unique_lock<std::mutex> lockEngine() {
    return std::unique_lock<std::mutex>(mutex_);
  }
  void signalShutdown() {
    running_ = false;
  }

  // Returns the terrain location at the given screen coord.  If the coord
  // is not on the map returns glm::vec3(HUGE_VAL).
  glm::vec3 screenToTerrain(const glm::vec2 &screenCoord) const;

  // Returns the entity that scores the LOWEST with the given scoring function.
  // Scoring function should have signature float scorer(const GameEntity *);
  template <class T>
  const GameEntity * findEntity(T scorer) const;

 private:
  Renderer();

  void render();
  void startRender();
  void endRender();
  void renderMap();
  void renderEntityOverlay(ModelEntity *entity);
  void renderEntity(ModelEntity *entity);
  void renderUI();

  glm::vec3 screenToNDC(const glm::vec2 &screenCoord) const;
  void renderActor(const Actor *actor, glm::mat4 transform);
  glm::vec2 worldToMinimap(const glm::vec3 &mapPos);

  std::map<id_t, GameEntity *> entities_;
  std::atomic<id_t> nextEntityID_;

  Controller *controller_;

#ifdef USE_FMOD
  FMOD::System *system_;
#endif  // USE_FMOD

  bool running_;

  std::mutex mutex_;

  glm::vec2 mapSize_;
  glm::vec4 mapColor_;

  glm::vec2 resolution_;
  Camera camera_;
  // render/simulation dt are scaled by this
  float timeMultiplier_;
  // Used to interpolate, last tick seen, and dt since last tick
  Clock::time_point lastTickTime_;
  float simdt_;
  // For updating purely render aspects
  Clock::time_point lastRender_;
  float renderdt_;
  float averageFPS_;
};

template <class T>
const GameEntity * Renderer::findEntity(T scorer) const {
  float bestscore = HUGE_VAL;
  const GameEntity *bestentity = nullptr;

  for (const auto& it : entities_) {
    const GameEntity *e = it.second;
    float score = scorer(e);
    if (score < bestscore) {
      bestscore = score;
      bestentity = e;
    }
  }

  return bestentity;
}
};  // namespace rts

#endif  // SRC_RTS_RENDERER_H_
