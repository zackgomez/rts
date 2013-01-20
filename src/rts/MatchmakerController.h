#ifndef SRC_RTS_MATCHMAKERCONTROLLER_H_
#define SRC_RTS_MATCHMAKERCONTROLLER_H_

#include "rts/Controller.h"

namespace rts {

class Matchmaker;

class MatchmakerController : public Controller {
 public:
   MatchmakerController(Matchmaker *mm);
   ~MatchmakerController();

   float getTimeElapsed() {
     return elapsedTime_;
   }

  virtual void initWidgets();
  virtual void clearWidgets();

  virtual void quitEvent();
  virtual void keyPress(SDL_keysym key);

  virtual void mouseDown(const glm::vec2 &screenCoord, int button) { }
  virtual void mouseUp(const glm::vec2 &screenCoord, int button) { }
  virtual void mouseMotion(const glm::vec2 &screenCoord) { }
  virtual void keyRelease(SDL_keysym key) { }

 protected:
  virtual void renderUpdate(float dt);

 private:
   Matchmaker *matchmaker_;
   float elapsedTime_;
};
};  // rts
#endif