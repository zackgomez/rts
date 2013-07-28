#ifndef SRC_RTS_CONTROLLER_H_
#define SRC_RTS_CONTROLLER_H_
#include <SDL/SDL.h>
#include "common/util.h"

class Shader;

namespace rts {

class ModelEntity;
class UI;

class Controller {
 public:
  Controller();
  virtual ~Controller();

  // @param dt the elapsed time in seconds since the last call
  void processInput(float dt);
  void render(float dt);

  virtual void onCreate() { }
  virtual void onDestroy() { }

	// TODO(zack): this is a bit hacky
	virtual void updateMapShader(Shader *shader) const { }

  //
  // Input handler functions
  //
  virtual void quitEvent() { }
  // @param button the SDL_BUTTON description of the pressed button
  virtual void mouseDown(const glm::vec2 &screenCoord, int button, int mods) { }
  // @param button the SDL_BUTTON description of the released button
  virtual void mouseUp(const glm::vec2 &screenCoord, int button, int mods) { }
  virtual void mouseMotion(const glm::vec2 &screenCoord) { }
  virtual void keyPress(int key, int mods) { }
  virtual void keyRelease(int key, int mods) { }

 protected:
  virtual void frameUpdate(float dt) { }
  virtual void renderExtra(float dt) { }

  UI *getUI() {
    return ui_;
  }

 private:
  UI *ui_;
};
};  // rts
#endif  // SRC_RTS_CONTROLLER_H_
