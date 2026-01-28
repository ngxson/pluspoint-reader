#pragma once

#include <Arduino.h>
#include <esp_partition.h>
#include <EpdFontData.h>
#include <EpdFont.h>
#include <GfxRenderer.h>
#include <SDCardManager.h>

class CustomFont {
 public:
  CustomFont(): mmap_data_(nullptr), font_(nullptr) {};
  ~CustomFont() {
    // unmap if needed
  };

  static constexpr uint32_t MAGIC = 0x02ABAB02;
  struct __attribute__((packed)) PackedData {
    uint32_t magic = 0;
    uint32_t version;
    char name[64];
    uint32_t offset;
    uint32_t size_bitmap;
    uint32_t size_glyphs;
    uint32_t size_intervals;
    // font data
    uint32_t intervalCount;
    int32_t advanceY;
    int32_t ascender;
    int32_t descender;
  };

  bool load() {
    // DEBUG
    tryFlashNewFont();
    FsFile file = SdMan.open("/no_custom_font", O_RDONLY);
    if (file && file.isOpen()) {
      Serial.printf("[%lu] [CF ] no_custom_font file present, skipping custom font load\n", millis());
      file.close();
      return false;
    }
    // END DEBUG

    const auto *p = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_FAT, nullptr);
    if (!p) {
      Serial.printf("[%lu] [CF ] No FAT partition --> disabled custom font\n", millis());
      return false;
    }

    auto err = esp_partition_read(p, 0, &data_, sizeof(data_));
    if (err != 0 || data_.magic != MAGIC) {
      Serial.printf("[%lu] [CF ] Custom font is invalid --> disabled custom font\n", millis());
      return false;
    }
    Serial.printf("[%lu] [CF ] data.offset = %u\n", millis(), data_.offset);
    Serial.printf("[%lu] [CF ] data.size_bitmap = %u\n", millis(), data_.size_bitmap);
    Serial.printf("[%lu] [CF ] data.size_glyphs = %u\n", millis(), data_.size_glyphs);
    Serial.printf("[%lu] [CF ] data.size_intervals = %u\n", millis(), data_.size_intervals);

    Serial.printf("[%lu] [CF ] Loading custom font '%s'\n", millis(), data_.name);
    err = esp_partition_mmap(p, 0, p->size, SPI_FLASH_MMAP_DATA, (const void**)&mmap_data_, &map_handle_);
    if (err != 0) {
      Serial.printf("[%lu] [CF ] Failed to mmap custom font, code: %d\n", millis(), err);
      return false;
    }

    const void* ptrBase = mmap_data_;
    const void* ptrBitmap = (const uint8_t*)ptrBase + data_.offset;
    const void* ptrGlyphs = (const uint8_t*)ptrBitmap + data_.size_bitmap;
    const void* ptrIntervals = (const uint8_t*)ptrGlyphs + data_.size_glyphs;

    /*
    Serial.printf("[%lu] [CF ] Custom font mmaped at %p, size: %u\n", millis(), ptrBase, p->size);
    Serial.printf("[%lu] [CF ] Custom font bitmap size: %u\n", millis(), data_.size_bitmap);
    Serial.printf("[%lu] [CF ] Custom font glyphs size: %u\n", millis(), data_.size_glyphs);
    Serial.printf("[%lu] [CF ] Custom font intervals size: %u\n", millis(), data_.size_intervals);

    Serial.printf("[%lu] [CF ] Base ptr: %p (%u)\n", millis(), ptrBase, ptrBase);
    Serial.printf("[%lu] [CF ] Bitmap ptr: %p (%u)\n", millis(), ptrBitmap, ptrBitmap);
    Serial.printf("[%lu] [CF ] Glyphs ptr: %p (%u)\n", millis(), ptrGlyphs, ptrGlyphs);
    Serial.printf("[%lu] [CF ] Intervals ptr: %p (%u)\n", millis(), ptrIntervals, ptrIntervals);
    */

    // correct pointers
    font_data_.bitmap = (const uint8_t*)(ptrBitmap);
    font_data_.glyph = (const EpdGlyph*)(ptrGlyphs);
    font_data_.intervals = (const EpdUnicodeInterval*)(ptrIntervals);
    font_data_.intervalCount = data_.intervalCount;
    font_data_.advanceY = static_cast<uint8_t>(data_.advanceY);
    font_data_.ascender = static_cast<int>(data_.ascender);
    font_data_.descender = static_cast<int>(data_.descender);
    font_data_.is2Bit = true; // TODO: store in header?
    Serial.printf("[%lu] [CF ] Custom font '%s' loaded\n", millis(), data_.name);

    // print font data for debugging
    Serial.printf("[%lu] [CF ] Custom font details:\n", millis());
    Serial.printf("[%lu] [CF ]   Ascender: %d\n", millis(), font_data_.ascender);
    Serial.printf("[%lu] [CF ]   Descender: %d\n", millis(), font_data_.descender);
    Serial.printf("[%lu] [CF ]   Advance Y: %u\n", millis(), font_data_.advanceY);
    Serial.printf("[%lu] [CF ]   Is 2-bit: %s\n", millis(), font_data_.is2Bit ? "true" : "false");
    Serial.printf("[%lu] [CF ]   Number of intervals: %u\n", millis(), font_data_.intervalCount);
    /*
    // print first 16 bytes of bitmap data for debugging
    Serial.printf("[%lu] [CF ] Custom font bitmap data (first 16 bytes):", millis());
    for (size_t i = 0; i < 16 && i < data_.size_bitmap; ++i) {
      Serial.printf(" %02X", data_.font.bitmap[i]);
    }
    // also print last 16 bytes of bitmap data for debugging
    Serial.printf("\n[%lu] [CF ] Custom font bitmap data (last 16 bytes):", millis());
    for (size_t i = (data_.size_bitmap > 16 ? data_.size_bitmap - 16 : 0); i < data_.size_bitmap; ++i) {
      Serial.printf(" %02X", data_.font.bitmap[i]);
    }
    Serial.printf("\n");

    // list first 5 glyphs for debugging
    Serial.printf("[%lu] [CF ] First 5 glyphs:\n", millis());
    for (size_t i = 0; i < 5 && i < (data_.size_glyphs / sizeof(EpdGlyph)); ++i) {
      const EpdGlyph& glyph = data_.font.glyph[i];
      Serial.printf("[%lu] [CF ] Glyph %u: w=%u, h=%u, advX=%u, left=%d, top=%d, dataLen=%u, dataOff=%u\n",
                    millis(), (unsigned)i, glyph.width, glyph.height, glyph.advanceX,
                    glyph.left, glyph.top, glyph.dataLength, glyph.dataOffset);
    }

    // list first 5 unicode intervals for debugging
    Serial.printf("[%lu] [CF ] First 5 unicode intervals:\n", millis());
    for (size_t i = 0; i < 5 && i < data_.size_intervals / sizeof(EpdUnicodeInterval); ++i) {
      const EpdUnicodeInterval& interval = data_.font.intervals[i];
      Serial.printf("[%lu] [CF ] Interval %u: first=0x%04X, last=0x%04X, offset=%u\n",
                    millis(), (unsigned)i, interval.first, interval.last, interval.offset);
    }
    */

    font_ = EpdFont(&font_data_);

    return true;
  }

  void tryFlashNewFont() {
    FsFile file = SdMan.open("/custom_font.bin", O_RDONLY);
    if (file && file.isOpen()) {
      Serial.printf("[%lu] [CF ] Found new custom font file to flash\n", millis());
      if (flashNewFont(file)) {
        Serial.printf("[%lu] [CF ] New custom font flashed successfully\n", millis());
      } else {
        Serial.printf("[%lu] [CF ] Failed to flash new custom font\n", millis());
      }
      file.close();
      SdMan.remove("/custom_font.bin");
    } else {
      Serial.printf("[%lu] [CF ] No custom_font.bin file to flash\n", millis());
    }
  }

  bool flashNewFont(FsFile& file) {
    const auto *p = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_FAT, nullptr);
    if (!p) {
      Serial.printf("[%lu] [CF ] Failed to find 'FAT' partition to flash new font\n", millis());
      return false;
    }

    size_t fileSize = file.size();
    if (fileSize > p->size) {
      Serial.printf("[%lu] [CF ] New font size %u exceeds partition size %u\n", millis(), fileSize, p->size);
      return false;
    }

    Serial.printf("[%lu] [CF ] Flashing new font of size %u bytes\n", millis(), fileSize);

    auto err = esp_partition_erase_range(p, 0, p->size);
    if (err != 0) {
      Serial.printf("[%lu] [CF ] Error erasing partition before flashing new font: %d\n", millis(), err);
      return false;
    }

    static constexpr size_t CHUNK_SIZE = 4096;
    uint8_t buffer[CHUNK_SIZE];
    size_t bytesFlashed = 0;
    while (bytesFlashed < fileSize) {
      size_t toRead = std::min(CHUNK_SIZE, fileSize - bytesFlashed);
      int bytesRead = file.read(buffer, toRead);
      if (bytesRead <= 0) {
        Serial.printf("[%lu] [CF ] Error reading font file to flash\n", millis());
        return false;
      }
      auto err = esp_partition_write(p, bytesFlashed, buffer, bytesRead);
      if (err != 0) {
        Serial.printf("[%lu] [CF ] Error writing font data to flash: %d\n", millis(), err);
        return false;
      }
      bytesFlashed += bytesRead;
    }

    Serial.printf("[%lu] [CF ] Successfully flashed new font (%u bytes)\n", millis(), bytesFlashed);
    return true;
  }

  EpdFont* getFont() {
    return &font_;
  }

 private:
  PackedData data_;
  EpdFontData font_data_;
  EpdFont font_;
  const void* mmap_data_;
  spi_flash_mmap_handle_t map_handle_;
};
