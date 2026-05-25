#pragma once

#ifndef SIMULATOR

#include <HalDisplay.h>
#include <os/hw/Display.h>

class X4Display : public Display {
 public:
  void begin() override { hal.begin(); }

  uint8_t* getFrameBuffer() override { return hal.getFrameBuffer(); }
  uint16_t getWidth() const override { return HalDisplay::DISPLAY_WIDTH; }
  uint16_t getHeight() const override { return HalDisplay::DISPLAY_HEIGHT; }
  uint16_t getWidthBytes() const override { return HalDisplay::DISPLAY_WIDTH_BYTES; }

  void displayBuffer(RefreshMode mode = FAST_REFRESH) override {
    hal.displayBuffer(convertMode(mode), turnOffScreen);
  }

  void refreshDisplay(RefreshMode mode = FAST_REFRESH) override {
    hal.refreshDisplay(convertMode(mode), turnOffScreen);
  }

 private:
  HalDisplay hal;

  static HalDisplay::RefreshMode convertMode(RefreshMode mode) {
    switch (mode) {
      case FULL_REFRESH: return HalDisplay::FULL_REFRESH;
      case HALF_REFRESH: return HalDisplay::HALF_REFRESH;
      case FAST_REFRESH: return HalDisplay::FAST_REFRESH;
      default:           return HalDisplay::FAST_REFRESH;
    }
  }
};

#endif  // SIMULATOR
