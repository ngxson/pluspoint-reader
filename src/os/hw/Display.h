#pragma once

#include <cassert>
#include <cstdint>

class Display {
  inline static Display* s_instance = nullptr;

 public:
  static Display& getInstance() {
    assert(s_instance != nullptr && "Os::boot() must be called before using Display");
    return *s_instance;
  }
  static void setInstance(Display* d) { s_instance = d; }


  enum RefreshMode {
    FULL_REFRESH,  // Full refresh with complete waveform
    HALF_REFRESH,  // Half refresh — balanced quality and speed
    FAST_REFRESH,  // Fast refresh using custom LUT
  };

  bool turnOffScreen = false;

  virtual ~Display() = default;

  virtual void begin() = 0;

  virtual uint8_t* getFrameBuffer() = 0;
  virtual uint16_t getWidth() const = 0;
  virtual uint16_t getHeight() const = 0;
  virtual uint16_t getWidthBytes() const = 0;

  // Push framebuffer contents to screen
  virtual void displayBuffer(RefreshMode mode = FAST_REFRESH) = 0;
  // Re-trigger a display refresh without re-writing RAM (useful for grayscale pipeline)
  virtual void refreshDisplay(RefreshMode mode = FAST_REFRESH) = 0;

};
