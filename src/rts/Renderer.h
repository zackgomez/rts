#ifndef SRC_RTS_RENDERER_H_
#define SRC_RTS_RENDERER_H_
#include <map>
#include <set>
#include <GL/glew.h>
#include <glm/glm.hpp>
#include "common/Clock.h"
#include "common/Logger.h"
#include "rts/GameEntity.h"

namespace rts {

class Actor;
class Controller;
class Game;
class Map;
class ModelEntity;
class UI;

struct MapHighlight;
struct ChatMessage;

class Renderer {
 public:
  ~Renderer();

  static Renderer* get();

  void setController(Controller *controller);
  void setUI(UI *ui);
  UI* getUI() {
    return ui_;
  }

  Controller *getController() const {
    return controller_;
  }
  std::unique_lock<std::mutex> lockEntities() {
    return std::unique_lock<std::mutex>(mutex_);
  }
  void signalShutdown() {
    running_ = false;
  }

  // Sets the last update time for inter/extrapolation purposes
  void setLastTickTime(const Clock::time_point &t) {
    lastTickTime_ = t;
  }
  void setTimeMultiplier(float t) {
    timeMultiplier_ = t;
  }
  void startMainloop();

  // eventually replace this with a set map geometry or something similar
  void setMapSize(const glm::vec2 &mapSize) {
    mapSize_ = mapSize;
  }
  void setMapColor(const glm::vec4 &mapColor) {
    mapColor_ = mapColor;
  }

  void addChatMessage(const std::string &message);

  float getSimDT() const {
    return simdt_;
  }
  const glm::vec2& getResolution() const {
    return resolution_;
  }
  const glm::vec3& getCameraPos() const {
    return cameraPos_;
  }
  void updateCamera(const glm::vec3 &delta);
  // moves the camera to where you click on the minimap
  void minimapUpdateCamera(const glm::vec2 &screenCoord);

  // returns 0 if no acceptable entity near click
  // TODO(zack): move selection to the UI/player
  id_t selectEntity(const glm::vec2 &screenCoord) const;
  std::set<id_t> selectEntities(const glm::vec3 &start,
                                const glm::vec3 &end, id_t pid) const;
  glm::vec2 convertUIPos(const glm::vec2 &pos) const;

  // Returns the terrain location at the given screen coord.  If the coord
  // is not on the map returns glm::vec3(HUGE_VAL).
  glm::vec3 screenToTerrain(const glm::vec2 &screenCoord) const;
  const std::map<const ModelEntity *, glm::vec3>& getEntityWorldPosMap() const;
  const std::vector<ChatMessage>& getChatMessages() const {
    return chats_;
  }

 private:
  Renderer();

  void render();
  void startRender(float dt);
  void endRender();
  void renderMap();
  void renderEntity(ModelEntity *entity);
  void renderUI();
  void renderActorInfo();

  glm::vec3 screenToNDC(const glm::vec2 &screenCoord) const;
  void renderActor(const Actor *actor, glm::mat4 transform);
  glm::vec2 worldToMinimap(const glm::vec3 &mapPos);

  Controller *controller_;
  UI* ui_;

  bool running_;

  std::mutex mutex_;

  glm::vec2 mapSize_;
  glm::vec4 mapColor_;

  glm::vec3 cameraPos_;
  glm::vec2 resolution_;
  // render/simulation dt are scaled by this
  float timeMultiplier_;
  // Used to interpolate, last tick seen, and dt since last tick
  Clock::time_point lastTickTime_;
  float simdt_;
  // For updating purely render aspects
  uint32_t lastRender_;
  float renderdt_;

  std::vector<ChatMessage> chats_;

  std::map<const ModelEntity *, glm::vec3> ndcCoords_;
};

struct ChatMessage {
  ChatMessage(const std::string &m, const Clock::time_point &t)
      : msg(m), time(t) { }

  std::string msg;
  Clock::time_point time;
};
};  // namespace rts

#endif  // SRC_RTS_RENDERER_H_
