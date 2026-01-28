#!/bin/bash

set -e

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

cd $SCRIPT_DIR/.. # project root

CUSTOM_FONT_PATH="ngxson/MomoSignature-Regular.ttf"
CUSTOM_FONT_NAME="MomoSignature-Regular"

mkdir -p build
python lib/EpdFont/scripts/fontconvert.py custom 12 $CUSTOM_FONT_PATH --2bit > build/custom_font.cpp

ADDED_CONTENT=$(cat << EOL

#include <vector>
#include <fstream>
#include <iostream>

#define CREATE_RESOURCES
#include "EpdFontCustom.h"

int main() {
  FsSimple fs;

  FsSimple::FileEntry fontEntry;
  std::vector<uint8_t> fontData;

  EpdFontCustom fontCustom;
  fontCustom.serializeFont(fontEntry, fontData, "$CUSTOM_FONT_NAME",
                           &custom, sizeof(customBitmaps), sizeof(customGlyphs), sizeof(customIntervals));

  std::cout << "Font data size: " << fontData.size() << " bytes" << std::endl;

  fs.beginCreate();
  const char* err = fs.addFileEntry(fontEntry, fontData.data());
  if (err) {
    printf("Error adding font file entry: %s\n", err);
    return 1;
  }

  std::cout << "Total resources size: " << fs.getWriteSize() << " bytes" << std::endl;

  std::ofstream out_file("build/resources.bin", std::ios::binary);
  out_file.write((const char*)fs.getWriteData(), fs.getWriteSize());
  out_file.close();

  return 0;
}
EOL
)

echo "$ADDED_CONTENT" >> build/custom_font.cpp

# replace #pragma once --> #pragma pack(8)
# sed -i '' 's/#pragma once/#pragma pack(8)/' build/custom_font.cpp
sed -i '' 's/#pragma once//' build/custom_font.cpp

# compile the C++ code to generate binary font file
g++ -o build/generate_custom_font build/custom_font.cpp -Ilib/EpdFont -Ilib/FsHelpers -std=c++17
./build/generate_custom_font
