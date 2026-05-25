#pragma once

#include <os/hw/Display.h>

#ifndef SIMULATOR
#include "activity/Activity.h"
#include "activity/ActivityManager.h"
#include "activity/ActivityResult.h"
#endif

namespace Os {
  // Must be called once at startup before any other OS component is used.
  // Creates the platform display, sets the Display singleton, and calls begin().
  void boot();
}
