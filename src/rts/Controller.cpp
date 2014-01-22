#define GLM_SWIZZLE
#include <functional>
#include "rts/Controller.h"
#include "rts/Input.h"
#include "rts/Renderer.h"
#include "rts/ResourceManager.h"
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
      [=] (const KeyEvent &ev) {
        if (ui_->handleKeyRelease(ev)) {
          return;
        }
        keyRelease(ev);
      },
      [=] (unsigned int unicode) {
        if (ui_->handleCharInput(unicode)) {
          return;
        }
        charInput(unicode);
      },
      std::bind(&Controller::quitEvent, this));

  frameUpdate(dt);
}

void Controller::render(float dt) {
  ui_->render(dt);

  renderExtra(dt);
}

void Controller::renderCursor(const std::string &cursor_tex_name) {

  auto mouse_state = getMouseState();
  glDisable(GL_DEPTH_TEST);
  const auto res = Renderer::get()->getResolution();
  const float cursor_size = std::min(res.x, res.y) * 0.05f;
  GLuint texture = ResourceManager::get()->getTexture(cursor_tex_name);
  drawTextureCenter(
      mouse_state.screenpos,
      glm::vec2(cursor_size),
      texture, glm::vec4(0, 0, 1, 1));
  glEnable(GL_DEPTH_TEST);
}
};  // rts
