#pragma once

#include <Arduino.h>

static HWCDC& UnwrappedSerial = Serial;

// Wrapper around Arduino Serial to avoid race conditions with the EmulationUtils
// When a transaction between device and host is ongoing, no other code should use Serial
class MySerialImpl : public Print {
 public:
  void begin(unsigned long baud) {
    UnwrappedSerial.begin(baud);
  }
  size_t write(uint8_t b) override;
  size_t write(const uint8_t* buffer, size_t size) override;
  void flush() override;
  operator bool() const {
    return UnwrappedSerial.operator bool();
  }
  static MySerialImpl instance;
};

// monkey-patch Serial to use MySerialImpl
#define Serial MySerialImpl::instance
