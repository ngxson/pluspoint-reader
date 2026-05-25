#pragma once

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <chrono>
#include <thread>

static inline unsigned long millis() {
  using namespace std::chrono;
  static const auto start = steady_clock::now();
  return (unsigned long)duration_cast<milliseconds>(steady_clock::now() - start).count();
}

static inline unsigned long micros() {
  using namespace std::chrono;
  static const auto start = steady_clock::now();
  return (unsigned long)duration_cast<microseconds>(steady_clock::now() - start).count();
}

static inline void delay(unsigned long ms) {
  std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

// ESP32 attribute stubs
#define RTC_NOINIT_ATTR
