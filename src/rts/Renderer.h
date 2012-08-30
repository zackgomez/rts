#pragma once
#include <map>
#include <GL/glew.h>
#include <glm/glm.hpp>
#include "Logger.h"
#include "Entity.h"
#include "Engine.h"

namespace rts {

class Map;
class LocalPlayer;
class Game;
class Actor;
struct MapHighlight;
class Effect;

class Renderer {
public:
  Renderer();
  ~Renderer();

  void setLocalPlayer(const LocalPlayer *p) {
    player_ = p;
  }

  void setGame(const Game *game) {
    game_ = game;
  }

  virtual void renderMessages(const std::set<Message> &messages);
  virtual void renderEntity(const Entity *entity);
  virtual void renderUI();
  virtual void renderMinimap();
  virtual void renderMap(const Map *map);

  virtual void startRender(float dt);
  virtual void endRender();

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
  id_t selectEntity (const glm::vec2 &screenCoord) const;
  std::set<id_t> selectEntities(const glm::vec3 &start,
                                const glm::vec3 &end, id_t pid) const;
  void setSelection(const std::set<id_t> &selection);

  // Returns the terrain location at the given screen coord.  If the coord
  // is not on the map returns glm::vec3(HUGE_VAL).
  glm::vec3 screenToTerrain (const glm::vec2 &screenCoord) const;

  void highlight(const glm::vec2 &mapCoord);
  void setDragRect(const glm::vec3 &start, const glm::vec3 &end);

  void displayChatBox(float time) { displayChatBoxTimer_ = time; }

  glm::vec2 convertUIPos(const glm::vec2 &pos);

private:
  glm::vec3 screenToNDC(const glm::vec2 &screenCoord) const;
  void renderActor(const Actor *actor, glm::mat4 transform);
  glm::vec2 worldToMinimap(const glm::vec3 &mapPos);

  // cached messages
  std::set<Message> messages_;
  const Game *game_;
  const LocalPlayer *player_;

  glm::vec3 cameraPos_;
  glm::vec3 lightPos_;
  glm::vec2 resolution_;
  // Used to interpolate, last tick seen, and dt since last tick
  float simdt_;
  // For updating purely render aspects
  uint32_t lastRender_;
  float renderdt_;
  
  float displayChatBoxTimer_;

  std::map<const Entity *, glm::vec3> ndcCoords_;
  std::map<const Entity *, glm::vec3> mapCoords_;
  std::set<id_t> selection_;

  std::vector<MapHighlight> highlights_;
  std::vector<Effect*> effects_;
  glm::vec3 dragStart_, dragEnd_;

  static LoggerPtr logger_;
};


struct MapHighlight {
  glm::vec2 pos;
  float remaining;
};

class Effect {
public:
  Effect() { }
  virtual ~Effect() { }

  virtual void render(float dt) = 0;
  virtual bool needsRemoval() const = 0;
};

class BloodEffect : public Effect {
public:
  BloodEffect(id_t aid);
  virtual ~BloodEffect();

  virtual void render(float dt);
  virtual bool needsRemoval() const;

private:
  id_t aid_;
  float t_;
};

}; // namespace rts
