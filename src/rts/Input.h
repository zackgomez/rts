#ifndef SRC_RTS_INPUT_H_
#define SRC_RTS_INPUT_H_

/**
 * This file contains a platform/library independent representation of input
 * events.
 *
 * Notes:
 * window_coord is in the range [0, 1]^2 with the origin in the top left.
 */
namespace rts {

enum class InputAction {
  INPUT_PRESS = 1,
  INPUT_RELEASE = 2,
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

struct MouseEvent {
  InputAction action;
  MouseButton button;
  int mods;
  glm::vec2 window_coord;
};

};  // rts
#endif  // SRC_RTS_INPUT_H_
