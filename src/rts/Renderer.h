#ifndef SRC_RTS_RENDERER_H_
#define SRC_RTS_RENDERER_H_
#include <atomic>
#include <map>
#include <mutex>
#include <set>
#include <GL/glew.h>
#include <glm/glm.hpp>
#include "common/Clock.h"
#include "common/Types.h"
#include "rts/Camera.h"
#ifdef USE_FMOD
#include <fmod.hpp>
#endif  // USE_FMOD

namespace rts {

class Controller;
class EffectManager;
class Map;
class ModelEntity;
class UI;

class Renderer {
 public:
  ~Renderer();

  static Renderer* get() {
    static Renderer instance;
    return &instance;
  }

  EffectManager* getEffectManager() {
    return effectManager_;
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

  std::map<id_t, ModelEntity *>& getEntities() {
    return entities_;
  }
  const std::map<id_t, ModelEntity *>& getEntities() const {
    return entities_;
  }
  ModelEntity * getEntity(id_t id) {
    auto it = entities_.find(id);
    if (it != entities_.end()) {
      return it->second;
    }
    return nullptr;
  }

  const ModelEntity * castRay(
      const glm::vec3 &origin,
      const glm::vec3 &dir,
      std::function<bool(const ModelEntity *)> filter) const;
  // Internally synchronized
  id_t newEntityID();
  void spawnEntity(ModelEntity *ent);
  void removeEntity(id_t eid);
  void clearEntities();

  float getRenderTime() {
    return Clock::secondsSince(startTime_);
  }
  void setTimeMultiplier(float t) {
    timeMultiplier_ = t;
  }
  void setGameTime(float t) {
    gameTime_ = t;
  }
  float getGameTime() const {
    return gameTime_;
  }
  // eventually replace this with a set map geometry or something similar
  void setMapSize(const glm::vec2 &mapSize) {
    mapSize_ = mapSize;
  }
  void setMapColor(const glm::vec4 &mapColor) {
    mapColor_ = mapColor;
  }
  void setCameraLookAt(const glm::vec3 &pos);
  void updateCamera(const glm::vec3 &delta);
  /*
   * Set camera rotation
   * @param rot.x theta rotation in degrees
   * @param rot.y phi rotation in degrees
   */
  void rotateCamera(const glm::vec2 &rot);
  void resetCameraRotation();
  void zoomCamera(float delta);

  typedef std::function<void()> PostableFunction;
  void postToMainThread(const PostableFunction& func);

  // Only run these functions from the main thread
  void setController(Controller *controller);
  void clearController();

  typedef std::function<void(const ModelEntity *, float)>
      EntityOverlayRenderer;
  void setEntityOverlayRenderer(EntityOverlayRenderer r) {
    entityOverlayRenderer_ = r;
  }


  void startMainloop();
  std::unique_lock<std::mutex> lockEngine() {
    return std::unique_lock<std::mutex>(mutex_);
  }
  void signalShutdown() {
    running_ = false;
  }

  // Returns the terrain location at the given screen coord.  If the coord
  // is not on the map returns glm::vec3(HUGE_VAL).
  std::tuple<glm::vec3, glm::vec3> screenToRay(const glm::vec2 &screenCoord) const;
  glm::vec3 screenToTerrain(const glm::vec2 &screenCoord) const;

  // Returns the entity that scores the LOWEST with the given scoring function.
  // Scoring function should have signature float scorer(const ModelEntity *);
  template <class T>
  const ModelEntity * findEntity(T scorer) const;

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
  glm::vec2 worldToMinimap(const glm::vec3 &mapPos);

  std::map<id_t, ModelEntity *> entities_;
  std::atomic<id_t> nextEntityID_;

  EffectManager *effectManager_;

  std::mutex funcMutex_;
  std::vector<PostableFunction> funcQueue_;
  Controller *controller_;
  EntityOverlayRenderer entityOverlayRenderer_;

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
  // 'game' time, affected by timeMultiplier
  float gameTime_;
  // For updating purely render aspects
  Clock::time_point lastRender_;
  Clock::time_point startTime_;
  float renderdt_;
  float averageFPS_;
};

template <class T>
const ModelEntity * Renderer::findEntity(T scorer) const {
  float bestscore = HUGE_VAL;
  const ModelEntity *bestentity = nullptr;

  for (const auto& it : entities_) {
    const ModelEntity *e = it.second;
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
