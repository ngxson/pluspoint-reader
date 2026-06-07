#include "os.h"

#include <os/graphic/Graphic.h>
#include <os/hw/Input.h>

#ifdef SIMULATOR
#include <drivers/sim/SimDisplay.h>
#include <drivers/sim/SimInput.h>
static SimDisplay platformDisplay;
static SimInput platformInput;
#else
#include <drivers/real/X4Display.h>
#include <drivers/real/X4GPIO.h>
static X4Display platformDisplay;
static X4GPIO platformInput;
#endif

namespace Os {

void boot() {
  Display::setInstance(&platformDisplay);
  Input::setInstance(&platformInput);
  Display::getInstance().begin();
  Graphic::getInstance();
}

}  // namespace Os
