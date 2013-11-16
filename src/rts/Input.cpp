#include "rts/Input.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "common/Logger.h"
#include "rts/Graphics.h"

namespace rts {

static GLFWwindow * get_glfw_window() {
  return (GLFWwindow *) get_native_window_handle();
}

static int getMouseButtonState() {
  int ret = 0;
  for (int i = 0; i < 3; i++) {
    if (glfwGetMouseButton(get_glfw_window(), GLFW_MOUSE_BUTTON_1 + i)) {
      ret |= 1 << i;
    }
  }
  return ret;
}

static int convert_glfw_key(int key) {
  switch (key) {
  case GLFW_KEY_ESCAPE:
    return INPUT_KEY_ESC;
  case GLFW_KEY_ENTER:
    return INPUT_KEY_RETURN;
  case GLFW_KEY_BACKSPACE:
    return INPUT_KEY_BACKSPACE;
  case GLFW_KEY_LEFT_SHIFT:
    return INPUT_KEY_LEFT_SHIFT;
  case GLFW_KEY_RIGHT_SHIFT:
    return INPUT_KEY_RIGHT_SHIFT;
  case GLFW_KEY_LEFT_CONTROL:
    return INPUT_KEY_LEFT_CTRL;
  case GLFW_KEY_RIGHT_CONTROL:
    return INPUT_KEY_RIGHT_CTRL;
  case GLFW_KEY_LEFT_ALT:
    return INPUT_KEY_LEFT_ALT;
  case GLFW_KEY_RIGHT_ALT:
    return INPUT_KEY_RIGHT_ALT;

  case GLFW_KEY_UP:
    return INPUT_KEY_UP;
  case GLFW_KEY_DOWN:
    return INPUT_KEY_DOWN;
  case GLFW_KEY_LEFT:
    return INPUT_KEY_LEFT;
  case GLFW_KEY_RIGHT:
    return INPUT_KEY_RIGHT;

  case GLFW_KEY_F10:
    return INPUT_KEY_F10;

  // XXX(zack): this is a dirty hack
#ifdef MACOSX
  case GLFW_KEY_WORLD_1:
    return GLFW_KEY_GRAVE_ACCENT;
#endif
  }

  if (isprint(key)) {
    return key;
  }

  return INPUT_KEY_UNKNOWN;
}

static std::function<void(const glm::vec2 &, int)> mouse_down_handler;
static std::function<void(const glm::vec2 &, int)> mouse_up_handler;
static std::function<void(const glm::vec2 &, int)> mouse_motion_handler;
static std::function<void(const KeyEvent &)> key_press_handler;
static std::function<void(const KeyEvent &)> key_release_handler;

static void glfw_mouse_button_fun(GLFWwindow *w, int button, int action, int mods) {
  double x, y;
  glfwGetCursorPos(w, &x, &y);
  int my_button;
  switch (button) {
  case GLFW_MOUSE_BUTTON_1:
    my_button = MouseButton::LEFT;
    break;
  case GLFW_MOUSE_BUTTON_2:
    my_button = MouseButton::RIGHT;
    break;
  case GLFW_MOUSE_BUTTON_3:
    my_button = MouseButton::MIDDLE;
    break;
  default:
    return;
  }
  if (action == GLFW_PRESS) {
    mouse_down_handler(glm::vec2(x, y), my_button);
  } else {
    mouse_up_handler(glm::vec2(x, y), my_button);
  }
}

static void glfw_scroll_fun(GLFWwindow *w, double xoffset, double yoffset) {
  if (abs(yoffset) < 1) {
    return;
  }
  double x, y;
  glfwGetCursorPos(w, &x, &y);
  int button = yoffset > 0 ? WHEEL_UP : WHEEL_DOWN;
  mouse_down_handler(glm::vec2(x, y), button);
}

static void glfw_mouse_motion_fun(GLFWwindow *w, double x, double y) {
  mouse_motion_handler(glm::vec2(x, y), getMouseButtonState());
}

// TODO (key fun)
static void glfw_key_fun(
    GLFWwindow *window,
    int key,
    int scancode,
    int action,
    int mods) {
  KeyEvent event;
  event.key = convert_glfw_key(key);
  // TODO(zack): fill mods
  event.mods = 0;
  event.action = action == GLFW_PRESS
    ? InputAction::PRESS
    : InputAction::RELEASE;

  if (event.action == InputAction::PRESS) {
    key_press_handler(event);
  } else {
    key_release_handler(event);
  }
}


void handleEvents(
    std::function<void(const glm::vec2 &, int)> mouseDownHandler,
    std::function<void(const glm::vec2 &, int)> mouseUpHandler,
    std::function<void(const glm::vec2 &, int)> mouseMotionHandler,
    std::function<void(const KeyEvent &)> keyPressHandler,
    std::function<void(const KeyEvent &)> keyReleaseHandler,
    std::function<void()> quitEventHandler) {
  mouse_down_handler = mouseDownHandler;
  mouse_up_handler = mouseUpHandler;
  mouse_motion_handler = mouseMotionHandler;
  key_press_handler = keyPressHandler;
  key_release_handler = keyReleaseHandler;

  glfwSetMouseButtonCallback(get_glfw_window(), glfw_mouse_button_fun);
  glfwSetScrollCallback(get_glfw_window(), glfw_scroll_fun);
  glfwSetCursorPosCallback(get_glfw_window(), glfw_mouse_motion_fun);
  glfwSetKeyCallback(get_glfw_window(), glfw_key_fun);

  glfwPollEvents();
  if (glfwWindowShouldClose(get_glfw_window())) {
    quitEventHandler();
  }
}

MouseState getMouseState() {
  MouseState ret;
  double x, y;
  glfwGetCursorPos(get_glfw_window(), &x, &y);
  ret.screenpos = glm::vec2(x, y);
  ret.buttons = getMouseButtonState();
  // TODO fill this
  ret.mods = 0;

  return ret;
}

void hide_mouse_cursor() {
  glfwSetInputMode(get_glfw_window(), GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
}

void show_mouse_cursor() {
  glfwSetInputMode(get_glfw_window(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}

void grab_mouse() {
  // TODO
}

void release_mouse() {
  // TODO
}
};  // rts
