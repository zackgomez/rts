#ifndef SRC_RTS_INPUT_H_
#define SRC_RTS_INPUT_H_
#include <functional>
#include <glm/glm.hpp>

/**
 * This file contains a platform/library independent representation of input
 * events.
 */
namespace rts {

enum class InputAction {
  PRESS = 1,
  RELEASE = 2,
};

enum ModifierKeys {
  INPUT_MOD_SHIFT = 0x0001,
  INPUT_MOD_CONTROL = 0x0002,
  INPUT_MOD_ALT = 0x0004,
  INPUT_MOD_SUPER = 0x0008,
};

enum MouseButton {
  LEFT = 0x01,
  RIGHT = 0x02,
  MIDDLE = 0x04,

  WHEEL_UP = 0x08,
  WHEEL_DOWN = 0x10,
};

struct KeyEvent {
  InputAction action;
  int key;
  int mods;
};

enum KeyCodes {
  INPUT_KEY_UNKNOWN = 0,
  INPUT_KEY_A = 65,
  INPUT_KEY_B = 66,
  INPUT_KEY_C = 67,
  INPUT_KEY_D = 68,
  INPUT_KEY_E = 69,
  INPUT_KEY_F = 70,
  INPUT_KEY_G = 71,
  INPUT_KEY_H = 72,
  INPUT_KEY_I = 73,
  INPUT_KEY_J = 74,
  INPUT_KEY_K = 75,
  INPUT_KEY_L = 76,
  INPUT_KEY_M = 77,
  INPUT_KEY_N = 78,
  INPUT_KEY_O = 79,
  INPUT_KEY_P = 80,
  INPUT_KEY_Q = 81,
  INPUT_KEY_R = 82,
  INPUT_KEY_S = 83,
  INPUT_KEY_T = 84,
  INPUT_KEY_U = 85,
  INPUT_KEY_V = 86,
  INPUT_KEY_W = 87,
  INPUT_KEY_X = 88,
  INPUT_KEY_Y = 89,
  INPUT_KEY_Z = 90,

  INPUT_KEY_ESC = 256,
  INPUT_KEY_RETURN,
  INPUT_KEY_BACKSPACE,

  INPUT_KEY_LEFT_SHIFT,
  INPUT_KEY_RIGHT_SHIFT,
  INPUT_KEY_LEFT_CTRL,
  INPUT_KEY_RIGHT_CTRL,
  INPUT_KEY_LEFT_ALT,
  INPUT_KEY_RIGHT_ALT,

  INPUT_KEY_UP,
  INPUT_KEY_DOWN,
  INPUT_KEY_LEFT,
  INPUT_KEY_RIGHT,

  INPUT_KEY_F10,
};

struct MouseState {
  glm::vec2 screenpos;
  int buttons;
  int mods;
};

MouseState getMouseState();

void handleEvents(
    std::function<void(const glm::vec2 &, int)> mouseDownHandler,
    std::function<void(const glm::vec2 &, int)> mouseUpHandler,
    std::function<void(const glm::vec2 &, int)> mouseMotionHandler,
    std::function<void(const KeyEvent &)> keyPressHandler,
    std::function<void(const KeyEvent &)> keyReleaseHandler,
    std::function<void()> quitEventHandler);

void hide_mouse_cursor();
void show_mouse_cursor();

void grab_mouse();
void release_mouse();

};  // rts
#endif  // SRC_RTS_INPUT_H_
