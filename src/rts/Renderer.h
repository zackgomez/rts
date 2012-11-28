#ifndef SRC_RTS_RENDERER_H_
#define SRC_RTS_RENDERER_H_
#include <map>
#include <set>
#include <GL/glew.h>
#include <glm/glm.hpp>
#include "common/Logger.h"
#include "rts/Entity.h"

namespace rts {

class Map;
class LocalPlayer;
class Game;
class Actor;
struct MapHighlight;

class Renderer {
 public:
  Renderer();
  ~Renderer();

  void setLocalPlayer(const LocalPlayer *p) {
    player_ = p;
  }
  LocalPlayer * getLocalPlayer() const {
    return (LocalPlayer*) player_;
  }

  void setGame(const Game *game) {
    game_ = game;
  }

  void renderMessages(const std::set<Message> &messages);
  void renderEntity(const Entity *entity);
  void renderUI();
  void renderMinimap();
  void renderMap(const Map *map);

  void addChatMessage(id_t from, const std::string &message);

  void startRender(float dt);
  void endRender();

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
  id_t selectEntity(const glm::vec2 &screenCoord) const;
  std::set<id_t> selectEntities(const glm::vec3 &start,
                                const glm::vec3 &end, id_t pid) const;
  void setSelection(const std::set<id_t> &selection);

  // Returns the terrain location at the given screen coord.  If the coord
  // is not on the map returns glm::vec3(HUGE_VAL).
  glm::vec3 screenToTerrain(const glm::vec2 &screenCoord) const;

  void highlight(const glm::vec2 &mapCoord);
  void setDragRect(const glm::vec3 &start, const glm::vec3 &end);

  glm::vec2 convertUIPos(const glm::vec2 &pos);

 private:
  glm::vec3 screenToNDC(const glm::vec2 &screenCoord) const;
  void renderActor(const Actor *actor, glm::mat4 transform);
  glm::vec2 worldToMinimap(const glm::vec3 &mapPos);

  // cached messages
  std::set<Message> messages_;
  const Game *game_;
  const LocalPlayer *player_;
  const Map* map_;

  glm::vec3 cameraPos_;
  glm::vec3 lightPos_;
  glm::vec2 resolution_;
  // Used to interpolate, last tick seen, and dt since last tick
  float simdt_;
  // For updating purely render aspects
  uint32_t lastRender_;
  float renderdt_;

  std::vector<std::string> chats_;
  float displayChatBoxTimer_;

  std::map<const Entity *, glm::vec3> ndcCoords_;
  std::map<const Entity *, glm::vec3> mapCoords_;
  std::set<id_t> selection_;

  std::vector<MapHighlight> highlights_;
  glm::vec3 dragStart_, dragEnd_;

  static LoggerPtr logger_;
};


struct MapHighlight {
  glm::vec2 pos;
  float remaining;
};
};  // namespace rts

#endif  // SRC_RTS_RENDERER_H_
