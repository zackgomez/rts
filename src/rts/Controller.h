#ifndef SRC_RTS_CONTROLLER_H_
#define SRC_RTS_CONTROLLER_H_
#include <SDL/SDL.h>
#include "common/util.h"

namespace rts {

class Game;
class LocalPlayer;

// This class is basically just and input source, right now it's configured
// soley for SDL
class Controller {
 public:
  explicit Controller(LocalPlayer *player);
  ~Controller();

  // @param dt the elapsed time in seconds since the last call
  void processInput(float dt);

  //
  // Input handler functions
  //
  void quitEvent();
  // @param button the SDL_BUTTON description of the pressed button
  void mouseDown(const glm::vec2 &screenCoord, int button);
  // @param button the SDL_BUTTON description of the released button
  void mouseUp(const glm::vec2 &screenCoord, int button);
  void keyPress(SDL_keysym key);
  void keyRelease(SDL_keysym key);

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
  bool leftDrag_;
  bool leftDragMinimap_;
  glm::vec3 leftStart_;

  // TODO(zack): move to renderer/engine as a camera velocity
  glm::vec2 cameraPanDir_;

  // TODO(zack): remove? move to player?
  void setSelection(const std::set<id_t> &new_selection);
  std::set<id_t> selection_;

  // Called once per frame with render dt
  void renderUpdate(float dt);
};
};  // rts
#endif  // SRC_RTS_CONTROLLER_H_
