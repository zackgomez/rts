#include "rts/Input.h"
#include <SDL/SDL.h>

namespace rts {

int convertSDLMouseButton(int button) {
  int ret = 0;
  if (button & SDL_BUTTON(1)) {
    ret |= MouseButton::LEFT;
  }
  if (button & SDL_BUTTON(2)) {
    ret |= MouseButton::RIGHT;
  }

  if (button == SDL_BUTTON_LEFT) {
    ret = MouseButton::LEFT;
  }
  if (button == SDL_BUTTON_RIGHT) {
    ret = MouseButton::RIGHT;
  }
  if (button == SDL_BUTTON_WHEELUP) {
    ret = MouseButton::WHEEL_UP;
  }
  if (button == SDL_BUTTON_WHEELDOWN) {
    ret = MouseButton::WHEEL_DOWN;
  }

  return ret;
}

KeyEvent convertSDLKeyEvent(SDL_KeyboardEvent sdl_event) {
  KeyEvent ret;
  ret.action = sdl_event.type == SDL_KEYDOWN
    ? InputAction::INPUT_PRESS
    : InputAction::INPUT_RELEASE;
  // TODO(zack): fill this out
  ret.mods = 0;

  ret.key = 0;
  switch (sdl_event.keysym.sym) {
  case SDLK_ESCAPE:
    ret.key = INPUT_KEY_ESC;
    break;
  case SDLK_RETURN:
    ret.key = INPUT_KEY_RETURN;
    break;
  case SDLK_BACKSPACE:
    ret.key = INPUT_KEY_BACKSPACE;
    break;
  case SDLK_LSHIFT:
    ret.key = INPUT_KEY_LEFT_SHIFT;
    break;
  case SDLK_RSHIFT:
    ret.key = INPUT_KEY_RIGHT_SHIFT;
    break;
  case SDLK_LCTRL:
    ret.key = INPUT_KEY_LEFT_CTRL;
    break;
  case SDLK_RCTRL:
    ret.key = INPUT_KEY_RIGHT_CTRL;
    break;
  case SDLK_LALT:
    ret.key = INPUT_KEY_LEFT_ALT;
    break;
  case SDLK_RALT:
    ret.key = INPUT_KEY_RIGHT_ALT;
    break;
  case SDLK_DOWN:
    ret.key = INPUT_KEY_DOWN;
    break;
  case SDLK_UP:
    ret.key = INPUT_KEY_UP;
    break;
  case SDLK_LEFT:
    ret.key = INPUT_KEY_LEFT;
    break;
  case SDLK_RIGHT:
    ret.key = INPUT_KEY_RIGHT;
    break;
  case SDLK_F10:
    ret.key = INPUT_KEY_F10;
    break;
  default:
    int sym = sdl_event.keysym.sym;
    if (isalpha(sym)) {
      ret.key = toupper(sdl_event.keysym.sym);
    } else if (isprint(sdl_event.keysym.sym)) {
      ret.key = sdl_event.keysym.sym;
    }
  }

  return ret;
}

void handleEvents(
    std::function<void(const glm::vec2 &, int)> mouseDownHandler,
    std::function<void(const glm::vec2 &, int)> mouseUpHandler,
    std::function<void(const glm::vec2 &, int)> mouseMotionHandler,
    std::function<void(const KeyEvent &)> keyPressHandler,
    std::function<void(const KeyEvent &)> keyReleaseHandler,
    std::function<void()> quitEventHandler) {
  glm::vec2 screenCoord;
  SDL_Event event;

  while (SDL_PollEvent(&event)) {
    switch (event.type) {
    case SDL_KEYDOWN:
      keyPressHandler(convertSDLKeyEvent(event.key));
      break;
    case SDL_KEYUP:
      keyReleaseHandler(convertSDLKeyEvent(event.key));
      break;
    case SDL_MOUSEBUTTONDOWN:
      screenCoord = glm::vec2(event.button.x, event.button.y);
      mouseDownHandler(screenCoord, convertSDLMouseButton(event.button.button));
      break;
    case SDL_MOUSEBUTTONUP:
      screenCoord = glm::vec2(event.button.x, event.button.y);
      mouseUpHandler(screenCoord, convertSDLMouseButton(event.button.button));
      break;
    case SDL_MOUSEMOTION:
      screenCoord = glm::vec2(event.motion.x, event.motion.y);
      mouseMotionHandler(screenCoord, convertSDLMouseButton(event.motion.state));
      break;
    case SDL_QUIT:
      quitEventHandler();
      break;
    }
  }
}

MouseState getMouseState() {
  MouseState ret;
  int x, y;
  ret.buttons = convertSDLMouseButton(SDL_GetMouseState(&x, &y));
  ret.screenpos = glm::vec2(x, y);
  // TODO fill this
  ret.mods = 0;

  return ret;
}

void hide_mouse_cursor() {
  SDL_ShowCursor(0);
}

void show_mouse_cursor() {
  SDL_ShowCursor(1);
}

void grab_mouse() {
  SDL_WM_GrabInput(SDL_GRAB_ON);
}

void release_mouse() {
  SDL_WM_GrabInput(SDL_GRAB_OFF);
}
};  // rts
