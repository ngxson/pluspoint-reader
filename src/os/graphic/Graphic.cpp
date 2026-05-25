#include "Graphic.h"

#include <os/internal.h>
#include <os/hw/Display.h>
#include <EpdFontData.h>
#include <Utf8.h>

Graphic& Graphic::getInstance() {
  static Graphic instance(Display::getInstance());
  return instance;
}

void Graphic::setOrientation(Orientation o) { orientation = o; }
Graphic::Orientation Graphic::getOrientation() const { return orientation; }

// Translate logical (x,y) to physical panel coordinates — mirrors GfxRenderer::rotateCoordinates
static inline void rotateCoordinates(Graphic::Orientation orientation, int x, int y,
                                     int* phyX, int* phyY,
                                     uint16_t panelW, uint16_t panelH) {
  switch (orientation) {
    case Graphic::Portrait:
      *phyX = y;
      *phyY = panelH - 1 - x;
      break;
    case Graphic::LandscapeClockwise:
      *phyX = panelW - 1 - x;
      *phyY = panelH - 1 - y;
      break;
    case Graphic::PortraitInverted:
      *phyX = panelW - 1 - y;
      *phyY = x;
      break;
    case Graphic::LandscapeCounterClockwise:
      *phyX = x;
      *phyY = y;
      break;
  }
}

int Graphic::getWidth() const {
  switch (orientation) {
    case Portrait:
    case PortraitInverted:          return display.getHeight();
    case LandscapeClockwise:
    case LandscapeCounterClockwise: return display.getWidth();
  }
  return display.getWidth();
}

int Graphic::getHeight() const {
  switch (orientation) {
    case Portrait:
    case PortraitInverted:          return display.getWidth();
    case LandscapeClockwise:
    case LandscapeCounterClockwise: return display.getHeight();
  }
  return display.getHeight();
}

void Graphic::drawPixel(int x, int y, bool black) const {
  if (x < 0 || x >= getWidth() || y < 0 || y >= getHeight()) return;

  int phyX, phyY;
  rotateCoordinates(orientation, x, y, &phyX, &phyY, display.getWidth(), display.getHeight());

  const uint32_t byteIndex = static_cast<uint32_t>(phyY) * display.getWidthBytes() + (phyX / 8);
  const uint8_t bitPos = 7 - (phyX % 8);  // MSB first; 0 = black on E-Ink

  uint8_t* fb = display.getFrameBuffer();
  if (black) {
    fb[byteIndex] &= ~(1 << bitPos);
  } else {
    fb[byteIndex] |= (1 << bitPos);
  }
}

void Graphic::drawBox(int x, int y, int w, int h, BoxOpts opts) const {
  if (opts.fill) {
    for (int py = y; py < y + h; py++) {
      for (int px = x; px < x + w; px++) {
        drawPixel(px, py, opts.black);
      }
    }
    return;
  }

  const auto& b = opts.border;
  // top
  for (int t = 0; t < b.top; t++)
    for (int px = x; px < x + w; px++) drawPixel(px, y + t, opts.black);
  // bottom
  for (int t = 0; t < b.bottom; t++)
    for (int px = x; px < x + w; px++) drawPixel(px, y + h - 1 - t, opts.black);
  // left (excluding corners already drawn)
  for (int py = y + b.top; py < y + h - b.bottom; py++)
    for (int t = 0; t < b.left; t++) drawPixel(x + t, py, opts.black);
  // right (excluding corners already drawn)
  for (int py = y + b.top; py < y + h - b.bottom; py++)
    for (int t = 0; t < b.right; t++) drawPixel(x + w - 1 - t, py, opts.black);
}

const uint8_t* Graphic::getGlyphBitmap(const EpdFontData* fontData, const EpdGlyph* glyph) const {
  if (fontData->groups != nullptr) {
    const uint32_t glyphIndex = static_cast<uint32_t>(glyph - fontData->glyph);
    return decompressor.getBitmap(fontData, glyph, glyphIndex);
  }
  return &fontData->bitmap[glyph->dataOffset];
}

void Graphic::renderGlyph(const EpdFontFamily& font, uint32_t cp, int cursorX, int cursorY, bool black,
                           EpdFontFamily::Style style) const {
  const EpdGlyph* glyph = font.getGlyph(cp, style);
  if (!glyph) return;

  const EpdFontData* fontData = font.getData(style);
  const uint8_t* bitmap = getGlyphBitmap(fontData, glyph);
  if (!bitmap) return;

  const int outerBase = cursorY - glyph->top;   // screenY = outerBase + glyphY
  const int innerBase = cursorX + glyph->left;  // screenX = innerBase + glyphX

  if (fontData->is2Bit) {
    int pos = 0;
    for (int gy = 0; gy < glyph->height; gy++) {
      for (int gx = 0; gx < glyph->width; gx++, pos++) {
        const uint8_t byte = bitmap[pos >> 2];
        const uint8_t shift = static_cast<uint8_t>((3 - (pos & 3)) * 2);
        // font: 0=white,1=light gray,2=dark gray,3=black → invert: 0=black,3=white
        const uint8_t val = 3 - ((byte >> shift) & 0x3);
        if (val < 3) drawPixel(innerBase + gx, outerBase + gy, black);
      }
    }
  } else {
    int pos = 0;
    for (int gy = 0; gy < glyph->height; gy++) {
      for (int gx = 0; gx < glyph->width; gx++, pos++) {
        const uint8_t byte = bitmap[pos >> 3];
        if ((byte >> (7 - (pos & 7))) & 1) drawPixel(innerBase + gx, outerBase + gy, black);
      }
    }
  }
}

void Graphic::drawText(const char* text, int x, int y, TextOpts opts) const {
  if (!text || *text == '\0' || !opts.font) return;

  const EpdFontData* fontData = opts.font->getData(opts.style);
  const int cursorY = y + fontData->ascender;

  int lastBaseX = x;
  int lastBaseLeft = 0;
  int lastBaseWidth = 0;
  int lastBaseTop = 0;
  int32_t prevAdvanceFP = 0;
  uint32_t prevCp = 0;
  uint32_t cp;

  while ((cp = utf8NextCodepoint(reinterpret_cast<const uint8_t**>(&text)))) {
    if (utf8IsCombiningMark(cp)) {
      const EpdGlyph* g = opts.font->getGlyph(cp, opts.style);
      if (!g) continue;
      const int raiseBy = combiningMark::raiseAboveBase(g->top, g->height, lastBaseTop);
      const int cx = combiningMark::centerOver(lastBaseX, lastBaseLeft, lastBaseWidth, g->left, g->width);
      renderGlyph(*opts.font, cp, cx, cursorY - raiseBy, opts.black, opts.style);
      continue;
    }

    cp = opts.font->applyLigatures(cp, text, opts.style);

    if (prevCp != 0) {
      const int8_t kernFP = opts.font->getKerning(prevCp, cp, opts.style);
      lastBaseX += fp4::toPixel(prevAdvanceFP + kernFP);
    }

    const EpdGlyph* glyph = opts.font->getGlyph(cp, opts.style);
    lastBaseLeft = glyph ? glyph->left : 0;
    lastBaseWidth = glyph ? glyph->width : 0;
    lastBaseTop = glyph ? glyph->top : 0;
    prevAdvanceFP = glyph ? glyph->advanceX : 0;

    renderGlyph(*opts.font, cp, lastBaseX, cursorY, opts.black, opts.style);
    prevCp = cp;
  }
}

int Graphic::getTextWidth(const char* text, TextOpts opts) const {
  if (!text || !opts.font) return 0;
  int w = 0, h = 0;
  opts.font->getTextDimensions(text, &w, &h, opts.style);
  return w;
}

int Graphic::getLineHeight(const EpdFontFamily& font) const { return font.getData()->advanceY; }

int Graphic::getAscender(const EpdFontFamily& font) const { return font.getData()->ascender; }
