#pragma once

#include <EpdFontFamily.h>
#include <FontDecompressor.h>
#include <os/hw/Display.h>

struct BoxOpts {
  struct Border {
    uint8_t top = 1, right = 1, bottom = 1, left = 1;
    Border() = default;
    explicit Border(uint8_t all) : top(all), right(all), bottom(all), left(all) {}
  };
  bool fill = false;
  bool black = true;
  Border border;
};

struct TextOpts {
  const EpdFontFamily* font = nullptr;
  bool black = true;
  EpdFontFamily::Style style = EpdFontFamily::REGULAR;
};

class Graphic {
 public:
  // Mirrors GfxRenderer::Orientation — logical screen orientation from caller's perspective
  enum Orientation {
    Portrait,                  // 480x800 logical (90° CW from physical panel) — device default
    LandscapeClockwise,        // 800x480 logical, 180° from panel
    PortraitInverted,          // 480x800 logical, 90° CCW from panel
    LandscapeCounterClockwise, // 800x480 logical, native panel orientation
  };

  explicit Graphic(Display& display) : display(display) {}
  ~Graphic() = default;

  static Graphic& getInstance();

  void setOrientation(Orientation o);
  Orientation getOrientation() const;

  int getWidth() const;
  int getHeight() const;

  void drawBox(int x, int y, int w, int h, BoxOpts opts = {}) const;
  void drawText(const char* text, int x, int y, TextOpts opts) const;
  int getTextWidth(const char* text, TextOpts opts) const;
  int getLineHeight(const EpdFontFamily& font) const;
  int getAscender(const EpdFontFamily& font) const;

 private:
  Display& display;
  Orientation orientation = Portrait;
  mutable FontDecompressor decompressor;

  void drawPixel(int x, int y, bool black) const;
  const uint8_t* getGlyphBitmap(const EpdFontData* fontData, const EpdGlyph* glyph) const;
  void renderGlyph(const EpdFontFamily& font, uint32_t cp, int cursorX, int cursorY, bool black,
                   EpdFontFamily::Style style) const;
};
