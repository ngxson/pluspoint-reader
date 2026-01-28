#pragma once

#include <Arduino.h>
#include "EpdFontCustom.h"

bool EpdFontCustom::load(FsSimple& resources) {
  const auto* root = resources.getRoot();
  if (!root) {
    Serial.printf("[%lu] [FC ] Resource is not mounted, skipping\n", millis());
    return false;
  }

  // find font file entry
  const FsSimple::FileEntry* fontEntry = nullptr;
  for (size_t i = 0; i < FsSimple::MAX_FILES; i++) {
    const auto& entry = root->entries[i];
    if (entry.type == FsSimple::FILETYPE_FONT_REGULAR) {
      fontEntry = &entry;
      break;
    }
  }
  if (!fontEntry) {
    Serial.printf("[%lu] [FC ] No font found in resources, skipping\n", millis());
    return false;
  }

  // load font data
  Serial.printf("[%lu] [FC ] Loading custom font '%s'\n", millis(), fontEntry->name);
  data_ = (const Header*)resources.mmap(fontEntry);
  assert(data_ != nullptr);

  font_data_.bitmap = (const uint8_t*)data_ + data_->offsetBitmap;
  font_data_.glyph = (const EpdGlyph*)((const uint8_t*)data_ + data_->offsetGlyphs);
  font_data_.intervals = (const EpdUnicodeInterval*)((const uint8_t*)data_ + data_->offsetIntervals);
  font_data_.intervalCount = data_->intervalCount;
  font_data_.advanceY = data_->advanceY;
  font_data_.ascender = data_->ascender;
  font_data_.descender = data_->descender;
  font_data_.is2Bit = data_->is2Bit;

  font_ = EpdFont(&font_data_);
  Serial.printf("[%lu] [FC ] Custom font loaded successfully\n", millis());
  return true;
}

bool EpdFontCustom::valid() const {
  return data_ != nullptr;
}

const EpdFont* EpdFontCustom::getFont() const {
  return &font_;
}
