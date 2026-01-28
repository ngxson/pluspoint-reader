#pragma once

#include <FsSimple.h>
#include <EpdFontData.h>
#include <EpdFont.h>
#ifdef CREATE_RESOURCES
#include <vector>
#include <cstring>
#endif

class EpdFontCustom {
 public:
  struct __attribute__((packed)) Header {
    uint32_t offsetBitmap;
    uint32_t offsetGlyphs;
    uint32_t offsetIntervals;
    // font data
    uint32_t intervalCount;
    uint8_t advanceY;
    int32_t ascender;
    int32_t descender;
    uint8_t is2Bit;
    uint8_t reserved[32]; // reserved for future use
  };

  bool load(FsSimple& resources);
  bool valid() const;
  const EpdFont* getFont() const;

#ifdef CREATE_RESOURCES
  void serializeFont(FsSimple::FileEntry& outEntry, std::vector<uint8_t>& outData, const char* name,
                     const EpdFontData* data, size_t bitmapSize, size_t glyphsSize, size_t intervalsSize) {
    // prepare header
    Header header;
    header.intervalCount = data->intervalCount;
    header.advanceY = data->advanceY;
    header.ascender = data->ascender;
    header.descender = data->descender;
    header.is2Bit = data->is2Bit;

    // calculate offsets
    size_t offset = sizeof(Header);
    header.offsetBitmap = offset;
    offset += bitmapSize; // bitmap will be appended later
    header.offsetGlyphs = offset;
    offset += glyphsSize;
    header.offsetIntervals = offset;
    offset += intervalsSize;

    // copy header
    outData.clear();
    outData.reserve(sizeof(Header) + bitmapSize + glyphsSize + intervalsSize);
    outData.insert(outData.end(), (uint8_t*)&header, (uint8_t*)&header + sizeof(Header));
    outData.insert(outData.end(), data->bitmap, data->bitmap + bitmapSize);
    outData.insert(outData.end(), (uint8_t*)data->glyph, (uint8_t*)data->glyph + glyphsSize);
    outData.insert(outData.end(), (uint8_t*)data->intervals, (uint8_t*)data->intervals + intervalsSize);
    
    // add padding if necessary
    size_t padding = FsSimple::get_padding(outData.size());
    for (size_t i = 0; i < padding; i++) {
      outData.push_back(0);
    }

    // prepare file entry
    outEntry.type = FsSimple::FILETYPE_FONT_REGULAR;
    outEntry.size = outData.size();
    strncpy(outEntry.name, name, FsSimple::MAX_FILE_NAME_LENGTH);
  }
#endif

 private:
  const Header* data_;
  EpdFontData font_data_;
  EpdFont font_ = EpdFont(nullptr);
};
