#define GLM_SWIZZLE_XYZW
#include <functional>
#include "rts/Controller.h"
#include "rts/Input.h"
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

  auto mouse_state = getMouseState();
  ui_->update(mouse_state.screenpos, mouse_state.buttons);

  handleEvents(
      [=] (const glm::vec2 &p, int button) {
        if (ui_->handleMousePress(p, button)) {
          return;
        }
        mouseDown(p, button);
      },
      std::bind(&Controller::mouseUp, this, _1, _2),
      [=] (const glm::vec2 &pos, int buttons) {
        mouseMotion(pos);
      },
      [=] (const KeyEvent &ev) {
        if (ui_->handleKeyPress(ev)) {
          return;
        }
        keyPress(ev);
      },
      std::bind(&Controller::keyRelease, this, _1),
      std::bind(&Controller::quitEvent, this));

  frameUpdate(dt);
}

void Controller::render(float dt) {
  ui_->render(dt);

  renderExtra(dt);
}
};  // rts
