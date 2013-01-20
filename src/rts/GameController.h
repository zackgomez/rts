#ifndef SRC_RTS_GAMECONTROLLER_H_
#define SRC_RTS_GAMECONTROLLER_H_

#include "rts/Controller.h"

namespace rts {

class LocalPlayer;

struct MapHighlight {
  glm::vec2 pos;
  float remaining;
};

class GameController : public Controller {
 public:
  explicit GameController(LocalPlayer *player);
  ~GameController();

  virtual void initWidgets();
  virtual void clearWidgets();

  virtual void quitEvent();
  virtual void mouseDown(const glm::vec2 &screenCoord, int button);
  virtual void mouseUp(const glm::vec2 &screenCoord, int button);
  virtual void mouseMotion(const glm::vec2 &screenCoord);
  virtual void keyPress(SDL_keysym key);
  virtual void keyRelease(SDL_keysym key);

  // Accessors
  // returns glm::vec4(HUGE_VAL) for no rect, or glm::vec4(start, end)
  glm::vec4 getDragRect() const;

 protected:
  virtual void renderUpdate(float dt);

 private:
  //
  // Member variables
  //
  LocalPlayer *player_;

  bool chatting_;
  std::string message_;

  std::string state_;
  std::string order_;

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

  // TODO(zack): move to renderer/engine as a camera velocity
  glm::vec2 cameraPanDir_;
  float zoom_;


  void minimapUpdateCamera(const glm::vec2 &screenCoord);
  // returns NO_ENTITY if no acceptable entity near click
  id_t selectEntity(const glm::vec2 &screenCoord) const;
  std::set<id_t> selectEntities(const glm::vec2 &start,
      const glm::vec2 &end, id_t pid) const;
  void highlight(const glm::vec2 &mapCoord);
  void highlightEntity(id_t eid);
};
};  // rts

#endif  // SRC_RTS_GAMECONTROLLER_H_