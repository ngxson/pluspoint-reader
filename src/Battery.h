#pragma once
#ifndef EMULATED
#include <BatteryMonitor.h>

#define BAT_GPIO0 0  // Battery voltage

static BatteryMonitor battery(BAT_GPIO0);
#else
class BatteryMonitor {
 public:
  explicit BatteryMonitor() {}
  int readPercentage() const { return 100; } // Always return full battery in emulation
};
static BatteryMonitor battery;
#endif