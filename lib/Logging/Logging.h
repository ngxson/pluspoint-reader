#pragma once

#ifdef SIMULATOR

#include <cstdio>
#include <string>

#define LOG_ERR(origin, fmt, ...) fprintf(stderr, "[ERR] [" origin "] " fmt "\n", ##__VA_ARGS__)
#define LOG_INF(origin, fmt, ...) fprintf(stderr, "[INF] [" origin "] " fmt "\n", ##__VA_ARGS__)
#define LOG_DBG(origin, fmt, ...) fprintf(stderr, "[DBG] [" origin "] " fmt "\n", ##__VA_ARGS__)

inline std::string getLastLogs() { return {}; }
inline void clearLastLogs() {}
inline bool sanitizeLogHead() { return false; }

#else  // ---- ESP32 / Arduino ----

#include <HardwareSerial.h>

#include <string>

// For raw Serial access (e.g., binary data), use logSerial directly.
static HWCDC &logSerial = Serial;

void logPrintf(const char *level, const char *origin, const char *format, ...);

#define LOG_ERR(origin, format, ...)                                           \
  logPrintf("ERR", origin, format "\n", ##__VA_ARGS__)
#define LOG_INF(origin, format, ...)                                           \
  logPrintf("INF", origin, format "\n", ##__VA_ARGS__)
#define LOG_DBG(origin, format, ...)                                           \
  logPrintf("DBG", origin, format "\n", ##__VA_ARGS__)

std::string getLastLogs();
void clearLastLogs();
// Validates the RTC log state (magic word + logHead range). Returns true if
// corruption was detected (magic mismatch or logHead out of range), meaning
// logMessages is untrusted garbage. Callers should call clearLastLogs() when
// this returns true so getLastLogs() does not dump corrupt data into crash
// reports.
bool sanitizeLogHead();

class MySerialImpl : public Print {
public:
  void begin(unsigned long baud) { logSerial.begin(baud); }

  // Support boolean conversion for compatibility with code like:
  //   if (Serial) or while (!Serial)
  operator bool() const { return logSerial; }

  __attribute__((deprecated("Use LOG_* macro instead"))) size_t
  printf(const char *format, ...);
  size_t write(uint8_t b) override;
  size_t write(const uint8_t *buffer, size_t size) override;
  void flush() override;
  static MySerialImpl instance;
};

#ifdef Serial
#undef Serial
#endif
#define Serial MySerialImpl::instance

#endif  // SIMULATOR
