#define GLM_SWIZZLE_XYZW
#include <functional>
#include "rts/Controller.h"
#include "rts/UI.h"

namespace rts {

Controller::Controller() {
  ui_ = new UI();
}

Controller::~Controller() {
  delete ui_;
}

void Controller::processInput(float dt) {
  using namespace std::placeholders;

  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    interpretSDLEvent(
      event,
      [=] (const glm::vec2 &p, int button) {
        if (ui_->handleMousePress(p, button)) {
          return;
        }
        mouseDown(p, button);
      },
      std::bind(&Controller::mouseUp, this, _1, _2),
      std::bind(&Controller::mouseMotion, this, _1),
      [=] (SDL_keysym keysym) {
        if (ui_->handleKeyPress(keysym)) {
          return;
        }
        keyPress(keysym);
      },
      std::bind(&Controller::keyRelease, this, _1),
      std::bind(&Controller::quitEvent, this));
  }
  frameUpdate(dt);
}

void Controller::render(float dt) {
  ui_->render(dt);
}
};  // rts
