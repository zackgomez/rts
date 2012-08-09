#pragma once
#include <GL/glew.h>
#include <map>
#include "glm.h"
#include "Logger.h"
#include "Entity.h"
#include "Engine.h"

namespace rts {

class Map;
class LocalPlayer;
class Game;
class Actor;
struct MapHighlight;

class Renderer {
public:
  Renderer() : game_(NULL) { }
  virtual ~Renderer() { }

  void setGame(const Game *game) {
    game_ = game;
  }

  // Called before anything other render* functions are called
  virtual void renderMessages(const std::set<Message> &messages) = 0;
  virtual void renderEntity(const Entity *entity) = 0;
  virtual void renderMap(const Map *map) = 0;

  virtual void startRender(float dt) = 0;
  virtual void endRender() = 0;

protected:
  const Game *game_;
};

class OpenGLRenderer : public Renderer {
public:
  OpenGLRenderer(const glm::vec2 &resolution);
  ~OpenGLRenderer();

  virtual void renderMessages(const std::set<Message> &messages);
  virtual void renderEntity(const Entity *entity);
  virtual void renderUI();
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

private:
  glm::vec3 screenToNDC(const glm::vec2 &screenCoord) const;
  void renderActor(const Actor *actor, glm::mat4 transform);

  // cached messages
  std::set<Message> messages_;

  glm::vec3 cameraPos_;
  glm::vec3 lightPos_;
  glm::vec2 resolution_;
  // Used to interpolate, last tick seen, and dt since last tick
  tick_t lastTick_;
  float simdt_;
  // For updating purely render aspects
  float renderdt_;

  // TODO(zack) shader map
  GLuint mapProgram_;
  GLuint meshProgram_;
  std::map<std::string, Mesh *> meshes_;

  std::map<const Entity *, glm::vec3> ndcCoords_;
  std::set<id_t> selection_;

  std::vector<MapHighlight> highlights_;
  glm::vec3 dragStart_, dragEnd_;

  static LoggerPtr logger_;
};


struct MapHighlight {
  glm::vec2 pos;
  float remaining;
};
}; // namespace rts

