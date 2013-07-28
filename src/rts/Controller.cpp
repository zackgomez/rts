#define GLM_SWIZZLE_XYZW
#include <functional>
#include "rts/Controller.h"
#include "rts/Renderer.h"
#include "rts/UI.h"

namespace rts {

Controller::Controller() {
  ui_ = new UI();
}

Controller::~Controller() {
  delete ui_;
}

static void glfw_key_callback(
		GLFWwindow *w,
		int key,
		int sc,
		int action,
		int mods) {
	auto *controller == Renderer::get()->getController();
	if (!controller) {
		return;
	}

	if (action == GLFW_PRESS) {
		if (controller->getUI()->handleKeyPress(key, mods)) {
			return;
		}
		controller->keyPress(key, mods);
	} else if (action == GLFW_RELEASE) {
		if (controller->getUI()->handleKeyRelease(key, mods)) {
			return;
		}
		controller->keyRelease(key, mods);
	}
	// NOTE(zack): ignoring GLFW_REPEAT
}

static void glfw_mouse_button_callback(
		GLFWwindow *w,
		int button,
		int action,
		int mods) {
	auto *controller == Renderer::get()->getController();
	if (!controller) {
		return;
	}
	glm::vec2 screen_coord;
	glfwGetCursorPos(w, &screen_coord.x, &screen_coord.y);

	if (action == GLFW_PRESS) {
		if (controller->getUI()->handleMousePress(screen_coord, button, mods)) {
			return;
		}
		controller->mouseDown(screen_coord, key, mods);
	} else if (action == GLFW_RELEASE) {
		controller->mouseUp(screen_coord, button, mods);
	}
}

void Controller::processInput(float dt) {
  using namespace std::placeholders;

	glm::vec2 screen_coord;
	glfwGetCursorPos(getGLFWWindow(), &screen_coord.x, &screen_coord.y);
  buttons = SDL_GetMouseState(&x, &y);
  ui_->update(glm::vec2(x, y), buttons);

	glfwPollEvents();
	if (glfwShouldWindowClose()) {
		quitEvent();
	}

  frameUpdate(dt);
}

void Controller::render(float dt) {
  ui_->render(dt);

  renderExtra(dt);
}
};  // rts
