#!/bin/bash

set -e

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

cd $SCRIPT_DIR/.. # project root

CUSTOM_FONT_PATH="ngxson/MomoSignature-Regular.ttf"
CUSTOM_FONT_NAME="MomoSignature-Regular"

mkdir -p build
python lib/EpdFont/scripts/fontconvert.py custom 12 $CUSTOM_FONT_PATH --2bit > build/custom_font.cpp

ADDED_CONTENT=$(cat << EOL
#include <fstream>
#include <iostream>
#include <cstdint>
#include <cstring>

static constexpr uint32_t MAGIC = 0x02ABAB02;
#pragma pack(push, 1)
struct __attribute__((packed)) PackedData {
  uint32_t magic;
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
#pragma pack(pop)

size_t align64bits(size_t size) {
  return (size + 7) & ~7;
}

size_t getPaddingSize(size_t size) {
  int pad = (int)align64bits(size) - (int)size;
  if (pad < 0) {
    throw std::runtime_error("Negative padding size: " + std::to_string(pad));
  }
  return (size_t)pad;
}

int main() {
  std::ofstream out_file("build/custom_font.bin", std::ios::binary);

  PackedData data;
  data.magic = MAGIC;
  data.version = 1;

  strncpy(data.name, "$CUSTOM_FONT_NAME", sizeof(data.name) - 1);

  data.offset = align64bits(sizeof(PackedData));
  data.size_bitmap = align64bits(sizeof(customBitmaps));
  data.size_glyphs = align64bits(sizeof(customGlyphs));
  data.size_intervals = align64bits(sizeof(customIntervals));

  // font data
  data.intervalCount = custom.intervalCount;
  data.advanceY = custom.advanceY;
  data.ascender = custom.ascender;
  data.descender = custom.descender;

  std::cout << "offset: " << data.offset << std::endl;
  std::cout << "size_bitmap: " << data.size_bitmap << std::endl;
  std::cout << "size_glyphs: " << data.size_glyphs << std::endl;
  std::cout << "size_intervals: " << data.size_intervals << std::endl;

  std::cout << std::endl;

  size_t pad_header = getPaddingSize(sizeof(PackedData));
  size_t pad_bitmap = getPaddingSize(sizeof(customBitmaps));
  size_t pad_glyphs = getPaddingSize(sizeof(customGlyphs));

  std::cout << "pad_header: " << pad_header << std::endl;
  std::cout << "pad_bitmap: " << pad_bitmap << std::endl;
  std::cout << "pad_glyphs: " << pad_glyphs << std::endl;

  std::cout << std::endl;

  out_file.write(reinterpret_cast<const char*>(&data), sizeof(data));
  out_file.write(std::string(pad_header, '\0').c_str(), pad_header);
  std::cout << "after header, pos: " << out_file.tellp() << std::endl;

  out_file.write(reinterpret_cast<const char*>(customBitmaps), sizeof(customBitmaps));
  out_file.write(std::string(pad_bitmap, '\0').c_str(), pad_bitmap);
  std::cout << "after bitmap, pos: " << out_file.tellp() << std::endl;

  out_file.write(reinterpret_cast<const char*>(customGlyphs), sizeof(customGlyphs));
  out_file.write(std::string(pad_glyphs, '\0').c_str(), pad_glyphs);
  std::cout << "after glyphs, pos: " << out_file.tellp() << std::endl;

  out_file.write(reinterpret_cast<const char*>(customIntervals), sizeof(customIntervals));

  std::cout << "final size: " << out_file.tellp() << std::endl;

  return 0;
}
EOL
)

echo "$ADDED_CONTENT" >> build/custom_font.cpp

# replace #pragma once --> #pragma pack(8)
# sed -i '' 's/#pragma once/#pragma pack(8)/' build/custom_font.cpp
sed -i '' 's/#pragma once//' build/custom_font.cpp

# compile the C++ code to generate binary font file
g++ -o build/generate_custom_font build/custom_font.cpp -Ilib/EpdFont -std=c++17
./build/generate_custom_font
