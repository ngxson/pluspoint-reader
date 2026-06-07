#ifdef SIMULATOR

#include "SimInput.h"

#include <cstring>

SDL_Scancode SimInput::toScancode(Button button) {
  switch (button) {
    case Button::Back:        return SDL_SCANCODE_ESCAPE;
    case Button::Confirm:     return SDL_SCANCODE_RETURN;
    case Button::Left:        return SDL_SCANCODE_LEFT;
    case Button::Right:       return SDL_SCANCODE_RIGHT;
    case Button::Up:          return SDL_SCANCODE_UP;
    case Button::Down:        return SDL_SCANCODE_DOWN;
    case Button::Power:       return SDL_SCANCODE_ESCAPE;
    case Button::PageBack:    return SDL_SCANCODE_PAGEUP;
    case Button::PageForward: return SDL_SCANCODE_PAGEDOWN;
    default:                  return SDL_SCANCODE_UNKNOWN;
  }
}

void SimInput::update() {
  std::memcpy(prev, curr, sizeof(curr));
  const Uint8* state = SDL_GetKeyboardState(nullptr);
  for (int i = 0; i < 512; i++) {
    curr[i] = state[i] != 0;
  }
}

bool SimInput::isPressed(Button button) const {
  return curr[toScancode(button)];
}

bool SimInput::wasPressed(Button button) const {
  SDL_Scancode sc = toScancode(button);
  return curr[sc] && !prev[sc];
}

bool SimInput::wasReleased(Button button) const {
  SDL_Scancode sc = toScancode(button);
  return !curr[sc] && prev[sc];
}

bool SimInput::wasAnyPressed() const {
  for (int i = 0; i < 512; i++) {
    if (curr[i] && !prev[i]) return true;
  }
  return false;
}

bool SimInput::wasAnyReleased() const {
  for (int i = 0; i < 512; i++) {
    if (!curr[i] && prev[i]) return true;
  }
  return false;
}

#endif  // SIMULATOR
