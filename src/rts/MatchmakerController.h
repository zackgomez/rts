#ifndef SRC_RTS_MATCHMAKERCONTROLLER_H_
#define SRC_RTS_MATCHMAKERCONTROLLER_H_

#include "rts/Controller.h"

namespace rts {

class Matchmaker;
class CommandWidget;

class MatchmakerController : public Controller {
 public:
  MatchmakerController(Matchmaker *mm);
  ~MatchmakerController();

  float getTimeElapsed() {
    return elapsedTime_;
  }

	virtual void updateMapProgram(GLuint mapProgram) const;

  virtual void onCreate();
  virtual void onDestroy();

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
   CommandWidget *infoWidget_;
   float elapsedTime_;
};
};  // rts
#endif
