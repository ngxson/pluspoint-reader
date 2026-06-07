#pragma once

#ifdef SIMULATOR

#include <SDL2/SDL.h>
#include <os/hw/Input.h>

class SimInput : public Input {
 public:
  void update() override;
  bool wasPressed(Button button) const override;
  bool wasReleased(Button button) const override;
  bool isPressed(Button button) const override;
  bool wasAnyPressed() const override;
  bool wasAnyReleased() const override;

 private:
  // SDL_NUM_SCANCODES is 512
  bool prev[512] = {};
  bool curr[512] = {};

  static SDL_Scancode toScancode(Button button);
};

#endif  // SIMULATOR
