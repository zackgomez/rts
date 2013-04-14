#define GLM_SWIZZLE_XYZW
#include "rts/Controller.h"
#include "rts/UI.h"

namespace rts {
void Controller::processInput(float dt) {
  glm::vec2 screenCoord;
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    switch (event.type) {
    case SDL_KEYDOWN:
      if (UI::get()->handleKeyPress(event.key.keysym)) {
        break;
      }
      keyPress(event.key.keysym);
      break;
    case SDL_KEYUP:
      keyRelease(event.key.keysym);
      break;
    case SDL_MOUSEBUTTONDOWN:
      screenCoord = glm::vec2(event.button.x, event.button.y);
      if (UI::get()->handleMousePress(screenCoord, event.button.button)) {
        break;
      }
      mouseDown(screenCoord, event.button.button);
      break;
    case SDL_MOUSEBUTTONUP:
      screenCoord = glm::vec2(event.button.x, event.button.y);
      mouseUp(screenCoord, event.button.button);
      break;
    case SDL_MOUSEMOTION:
      screenCoord = glm::vec2(event.button.x, event.button.y);
      mouseMotion(screenCoord);
      break;
    case SDL_QUIT:
      quitEvent();
      break;
    }
  }
  renderUpdate(dt);
}
};  // rts
