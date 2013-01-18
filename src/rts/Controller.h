#ifndef SRC_RTS_CONTROLLER_H_
#define SRC_RTS_CONTROLLER_H_
#include <SDL/SDL.h>
#include "common/util.h"

namespace rts {

class Game;
class LocalPlayer;

class Controller {
 public:
  // @param dt the elapsed time in seconds since the last call
  void processInput(float dt);

  //
  // Input handler functions
  //
  virtual void quitEvent() = 0;
  // @param button the SDL_BUTTON description of the pressed button
  virtual void mouseDown(const glm::vec2 &screenCoord, int button) = 0;
  // @param button the SDL_BUTTON description of the released button
  virtual void mouseUp(const glm::vec2 &screenCoord, int button) = 0;
  virtual void mouseMotion(const glm::vec2 &screenCoord) = 0;
  virtual void keyPress(SDL_keysym key) = 0;
  virtual void keyRelease(SDL_keysym key) = 0;

 protected:
  virtual void renderUpdate(float dt) = 0;
};

// This class is basically just and input source, right now it's configured
// soley for SDL
class GameController : public Controller {
 public:
  explicit GameController(LocalPlayer *player);
  ~GameController();

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

  // TODO(zack): move to renderer/engine as a camera velocity
  glm::vec2 cameraPanDir_;
  float zoom_;
  void minimapUpdateCamera(const glm::vec2 &screenCoord);
  // returns NO_ENTITY if no acceptable entity near click
  id_t selectEntity(const glm::vec2 &screenCoord) const;
  std::set<id_t> selectEntities(const glm::vec2 &start,
      const glm::vec2 &end, id_t pid) const;
};
};  // rts
#endif  // SRC_RTS_CONTROLLER_H_
