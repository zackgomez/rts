#ifndef SRC_RTS_GAMECONTROLLER_H_
#define SRC_RTS_GAMECONTROLLER_H_

#include "rts/Controller.h"
#include <GL/glew.h>
#include "rts/GameScript.h"
#include "rts/UIAction.h"

namespace rts {

class LocalPlayer;
class GameEntity;
struct UIAction;

struct MapHighlight {
  glm::vec2 pos;
  float remaining;
};

class GameController : public Controller {
 public:
  explicit GameController(LocalPlayer *player);
  ~GameController();

  virtual void onCreate() override;
  virtual void onDestroy() override;

  virtual void quitEvent() override;
  virtual void mouseDown(const glm::vec2 &screenCoord, int button) override;
  virtual void mouseUp(const glm::vec2 &screenCoord, int button) override;
  virtual void mouseMotion(const glm::vec2 &screenCoord) override;
  virtual void keyPress(const KeyEvent &ev) override;
  virtual void keyRelease(const KeyEvent &ev) override;

  virtual void updateMapShader(Shader *shader) const;

  // Accessors
  // returns glm::vec4(HUGE_VAL) for no rect, or glm::vec4(start, end)
  glm::vec4 getDragRect() const;
  // GameController takes ownership of the data
  void setVisibilityData(void *data, size_t len, const glm::ivec2 &dim);
  GLuint getVisibilityTexture() const {
    return visTex_;
  }

 protected:
  virtual void frameUpdate(float dt) override;
  virtual void renderExtra(float dt) override;

 private:
  //
  // Member variables
  //
  LocalPlayer *player_;
  GameScript *gameScript_;

  std::string state_;
  std::string order_;
  UIAction action_;

  bool shift_;
  bool ctrl_;
  bool alt_;
  bool leftDrag_;
  bool leftDragMinimap_;
  glm::vec2 leftStart_;
  // For computing mouse motion
  glm::vec2 lastMousePos_;
  std::vector<MapHighlight> highlights_;
  std::map<id_t, float> entityHighlights_;

  bool renderNavMesh_;

  // TODO(zack): move to renderer/engine as a camera velocity
  glm::vec2 cameraPanDir_;
  float zoom_;

  void *visData_;
  size_t visDataLength_;
  glm::ivec2 visDim_;
  GLuint visTex_;

  v8::Handle<v8::Object> getJSController();
  void updateVisibility(float t);
  void minimapUpdateCamera(const glm::vec2 &screenCoord);
  void handleUIAction(const UIAction &action);
  // returns null if no acceptable entity near click
  GameEntity * selectEntity(const glm::vec2 &screenCoord) const;
  std::set<GameEntity *> selectEntities(
      const glm::vec2 &start,
      const glm::vec2 &end,
      id_t pid) const;
  void highlight(const glm::vec2 &mapCoord);
  void highlightEntity(id_t eid);
  Json::Value handleRightClick(
      const GameEntity *entity,
      const glm::vec3 &loc);

  std::string getCursorTexture() const;

  void attemptIssueOrder(Json::Value order);

  glm::vec4 getTeamColor(const id_t tid) const;
};
};  // rts

#endif  // SRC_RTS_GAMECONTROLLER_H_
