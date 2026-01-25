#pragma once

#include <Arduino.h>

static HWCDC& UnwrappedSerial = Serial;

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

#define Serial MySerialImpl::instance
