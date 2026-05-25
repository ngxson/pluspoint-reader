#ifdef SIMULATOR

#include <os/os.h>
#include <os/graphic/Fonts.h>
#include <os/graphic/Graphic.h>
#include <drivers/sim/SimDisplay.h>

int main() {
  Os::boot();
  Graphic& g = Graphic::getInstance();
  SimDisplay& simDisplay = static_cast<SimDisplay&>(Display::getInstance());

  // Clear to white
  g.drawBox(0, 0, g.getWidth(), g.getHeight(), BoxOpts{.fill = true, .black = false});

  // Draw "Hello, World!" centered
  const EpdFontFamily& font = getFontFamilyById(NOTOSANS_18_FONT_ID);
  const char* text = "Hello, World!";
  const TextOpts opts{.font = &font};
  const int tw = g.getTextWidth(text, opts);
  const int th = g.getLineHeight(font);
  const int x = (g.getWidth() - tw) / 2;
  const int y = (g.getHeight() - th) / 2;
  g.drawText(text, x, y, opts);

  simDisplay.displayBuffer();

  while (!simDisplay.shouldClose()) {
    simDisplay.pollEvents();
  }

  return 0;
}

#else  // ESP32

#include <Arduino.h>
void setup() {}
void loop() {}

#endif
