#pragma once

#ifndef SIMULATOR

#include <HalGPIO.h>
#include <os/hw/Input.h>

class X4GPIO : public Input {
 public:
  void update() override { hal.update(); }

  bool wasPressed(Button button) const override {
    return hal.wasPressed(toIndex(button));
  }
  bool wasReleased(Button button) const override {
    return hal.wasReleased(toIndex(button));
  }
  bool isPressed(Button button) const override {
    return hal.isPressed(toIndex(button));
  }
  bool wasAnyPressed() const override { return hal.wasAnyPressed(); }
  bool wasAnyReleased() const override { return hal.wasAnyReleased(); }

  HalGPIO& getHal() { return hal; }

 private:
  HalGPIO hal;

  static uint8_t toIndex(Button button) {
    switch (button) {
      case Button::Back:        return HalGPIO::BTN_BACK;
      case Button::Confirm:     return HalGPIO::BTN_CONFIRM;
      case Button::Left:        return HalGPIO::BTN_LEFT;
      case Button::Right:       return HalGPIO::BTN_RIGHT;
      case Button::Up:          return HalGPIO::BTN_UP;
      case Button::Down:        return HalGPIO::BTN_DOWN;
      case Button::Power:       return HalGPIO::BTN_POWER;
      case Button::PageBack:    return HalGPIO::BTN_LEFT;
      case Button::PageForward: return HalGPIO::BTN_RIGHT;
      default:                  return 0xFF;
    }
  }
};

#endif  // SIMULATOR
