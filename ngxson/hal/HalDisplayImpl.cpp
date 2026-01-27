#include <real/HalRealImpl.h>
#ifndef EMULATED
#include <EInkDisplay.h>
#endif
#include <HalDisplay.h>
#include <HalGPIO.h>
#include <EmulationUtils.h>
#include <string>

static uint8_t* emuFramebuffer0 = nullptr;

#ifndef EMULATED
HalDisplay::HalDisplay() : einkDisplay(EPD_SCLK, EPD_MOSI, EPD_CS, EPD_DC, EPD_RST, EPD_BUSY) {}
#else
HalDisplay::HalDisplay() {
  emuFramebuffer0 = new uint8_t[BUFFER_SIZE];
}
#endif

HalDisplay::~HalDisplay() {
#ifndef EMULATED
  // do nothing
#else
  delete[] emuFramebuffer0;
#endif
}

void HalDisplay::begin() {
#ifndef EMULATED
    einkDisplay.begin();
#else
    Serial.printf("[%lu] [   ] Emulated display initialized\n", millis());
    // no-op
#endif
}

void HalDisplay::clearScreen(uint8_t color) const {
#ifndef EMULATED
    einkDisplay.clearScreen(color);
#else
    Serial.printf("[%lu] [   ] Emulated clear screen with color 0x%02X\n", millis(), color);
    memset(emuFramebuffer0, color, BUFFER_SIZE);
#endif
}

void HalDisplay::drawImage(const uint8_t* imageData, uint16_t x, uint16_t y, uint16_t w, uint16_t h, bool fromProgmem) const {
#ifndef EMULATED
    einkDisplay.drawImage(imageData, x, y, w, h, fromProgmem);
#else
    Serial.printf("[%lu] [   ] Emulated draw image at (%u, %u) with size %ux%u\n", millis(), x, y, w, h);

    // Calculate bytes per line for the image
    const uint16_t imageWidthBytes = w / 8;

    // Copy image data to frame buffer
    for (uint16_t row = 0; row < h; row++) {
      const uint16_t destY = y + row;
      if (destY >= DISPLAY_HEIGHT)
        break;

      const uint16_t destOffset = destY * DISPLAY_WIDTH_BYTES + (x / 8);
      const uint16_t srcOffset = row * imageWidthBytes;

      for (uint16_t col = 0; col < imageWidthBytes; col++) {
        if ((x / 8 + col) >= DISPLAY_WIDTH_BYTES)
          break;

        if (fromProgmem) {
          emuFramebuffer0[destOffset + col] = pgm_read_byte(&imageData[srcOffset + col]);
        } else {
          emuFramebuffer0[destOffset + col] = imageData[srcOffset + col];
        }
      }
    }
#endif
}

void HalDisplay::displayBuffer(RefreshMode mode) {
#ifndef EMULATED
    einkDisplay.displayBuffer(static_cast<EInkDisplay::RefreshMode>(mode));
#else
    Serial.printf("[%lu] [   ] Emulated display buffer with mode %d\n", millis(), static_cast<int>(mode));
    EmulationUtils::Lock lock;
    EmulationUtils::sendDisplayData(reinterpret_cast<char*>(emuFramebuffer0), BUFFER_SIZE);
    EmulationUtils::recvRespInt64(); // dummy
#endif
}

void HalDisplay::refreshDisplay(RefreshMode mode, bool turnOffScreen) {
#ifndef EMULATED
    einkDisplay.refreshDisplay(static_cast<EInkDisplay::RefreshMode>(mode), turnOffScreen);
#else
    Serial.printf("[%lu] [   ] Emulated refresh display with mode %d, turnOffScreen %d\n", millis(), static_cast<int>(mode), turnOffScreen);
    // emulated delay
    if (mode == RefreshMode::FAST_REFRESH) {
      // delay(50);
    } else if (mode == RefreshMode::HALF_REFRESH) {
      delay(500);
    } else if (mode == RefreshMode::FULL_REFRESH) {
      delay(1200);
    }
#endif
}

void HalDisplay::deepSleep() {
#ifndef EMULATED
    einkDisplay.deepSleep();
#else
    Serial.printf("[%lu] [   ] Emulated deep sleep\n", millis());
    // no-op
#endif
}

uint8_t* HalDisplay::getFrameBuffer() const {
#ifndef EMULATED
    return einkDisplay.getFrameBuffer();
#else
    return emuFramebuffer0;
#endif
}

void HalDisplay::copyGrayscaleBuffers(const uint8_t* lsbBuffer, const uint8_t* msbBuffer) {
#ifndef EMULATED
    einkDisplay.copyGrayscaleBuffers(lsbBuffer, msbBuffer);
#else
    Serial.printf("[%lu] [   ] Emulated copy grayscale buffers\n", millis());
    // TODO: not sure what this does
#endif
}

void HalDisplay::copyGrayscaleLsbBuffers(const uint8_t* lsbBuffer) {
#ifndef EMULATED
    einkDisplay.copyGrayscaleLsbBuffers(lsbBuffer);
#else
    Serial.printf("[%lu] [   ] Emulated copy grayscale LSB buffers\n", millis());
    // TODO: not sure what this does
#endif
}

void HalDisplay::copyGrayscaleMsbBuffers(const uint8_t* msbBuffer) {
#ifndef EMULATED
    einkDisplay.copyGrayscaleMsbBuffers(msbBuffer);
#else
    Serial.printf("[%lu] [   ] Emulated copy grayscale MSB buffers\n", millis());
    // TODO: not sure what this does
#endif
}

void HalDisplay::cleanupGrayscaleBuffers(const uint8_t* bwBuffer) {
#ifndef EMULATED
    einkDisplay.cleanupGrayscaleBuffers(bwBuffer);
#else
    Serial.printf("[%lu] [   ] Emulated cleanup grayscale buffers\n", millis());
    // TODO: not sure what this does
#endif
}

void HalDisplay::displayGrayBuffer() {
#ifndef EMULATED
    einkDisplay.displayGrayBuffer();
#else
    Serial.printf("[%lu] [   ] Emulated display gray buffer\n", millis());
    // TODO: not sure what this does
#endif
}
