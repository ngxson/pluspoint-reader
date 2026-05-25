#include "os.h"

#include <os/graphic/Graphic.h>

#ifdef SIMULATOR
#include <drivers/sim/SimDisplay.h>
static SimDisplay platformDisplay;
#endif

namespace Os {

void boot() {
#ifdef SIMULATOR
  Display::setInstance(&platformDisplay);
#endif
  Display::getInstance().begin();
  Graphic::getInstance();
}

}  // namespace Os
