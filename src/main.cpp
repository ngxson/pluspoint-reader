#include <os/graphic/Fonts.h>
#include <os/graphic/Graphic.h>
#include <os/hw/Display.h>
#include <os/hw/Input.h>
#include <os/os.h>

#ifdef SIMULATOR
#include <drivers/sim/SimDisplay.h>
#endif

#ifndef SIMULATOR
#include <Arduino.h>
#endif

void setup() {
  Os::boot();
  Graphic& g = Graphic::getInstance();
  g.drawBox(0, 0, g.getWidth(), g.getHeight(), BoxOpts{.fill = true, .black = false});
  Display::getInstance().displayBuffer();
}

void loop() {
  Input& input = Input::getInstance();

#ifdef SIMULATOR
  static_cast<SimDisplay&>(Display::getInstance()).pollEvents();
#endif

  input.update();

  const char* key = nullptr;
  if (input.wasPressed(Input::Button::Up))    key = "Up";
  if (input.wasPressed(Input::Button::Down))  key = "Down";
  if (input.wasPressed(Input::Button::Left))  key = "Left";
  if (input.wasPressed(Input::Button::Right)) key = "Right";

  static bool firstFrame = true;
  if (firstFrame) {
    key = "Press a button";
    firstFrame = false;
  }

  if (key) {
    Graphic& g = Graphic::getInstance();
    g.drawBox(0, 0, g.getWidth(), g.getHeight(), BoxOpts{.fill = true, .black = false});
    const EpdFontFamily& font = getFontFamilyById(NOTOSANS_18_FONT_ID);
    const TextOpts opts{.font = &font};
    const int tw = g.getTextWidth(key, opts);
    const int th = g.getLineHeight(font);
    g.drawText(key, (g.getWidth() - tw) / 2, (g.getHeight() - th) / 2, opts);
    Display::getInstance().displayBuffer();
  }
}

#ifdef SIMULATOR
int main() {
  setup();
  SimDisplay& simDisplay = static_cast<SimDisplay&>(Display::getInstance());
  while (!simDisplay.shouldClose()) {
    loop();
  }
  return 0;
}
#endif
