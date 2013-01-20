#ifndef SRC_RTS_CONTROLLER_H_
#define SRC_RTS_CONTROLLER_H_
#include <SDL/SDL.h>
#include "common/util.h"

namespace rts {

class Controller {
 public:
   virtual ~Controller() { }
  // @param dt the elapsed time in seconds since the last call
  void processInput(float dt);

  virtual void onCreate() { }
  virtual void onDestroy() { }

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
};  // rts
#endif  // SRC_RTS_CONTROLLER_H_
