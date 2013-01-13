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
  void mouseMotion(const glm::vec2 &screenCoord);
  void keyPress(SDL_keysym key);
  void keyRelease(SDL_keysym key);

  // Accessors
  // returns glm::vec4(HUGE_VAL) for no rect, or glm::vec4(start, end)
  glm::vec4 getDragRect() const;

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

  // Called once per frame with render dt
  void renderUpdate(float dt);
  void minimapUpdateCamera(const glm::vec2 &screenCoord);
  // returns NO_ENTITY if no acceptable entity near click
  id_t selectEntity(const glm::vec2 &screenCoord) const;
  std::set<id_t> selectEntities(const glm::vec2 &start,
      const glm::vec2 &end, id_t pid) const;
};
};  // rts
#endif  // SRC_RTS_CONTROLLER_H_
