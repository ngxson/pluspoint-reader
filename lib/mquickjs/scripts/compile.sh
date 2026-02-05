#!/bin/bash

set -e

CURRENT_DIR=$(pwd)
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

BUILD_DIR="$SCRIPT_DIR/build"

cd $SCRIPT_DIR/.. # mquickjs directory

# args: input.js output.bin
INPUT="$1"
OUTPUT="$2"

if [ -z "$INPUT" ]; then
  echo "Usage: $0 <input.js> [<output.bin>]"
  exit 1
fi

if [ -z "$OUTPUT" ]; then
  OUTPUT="${INPUT%.*}.bin"
fi

TAG=ngxson/xteink_x4_mqjs_compiler
# docker build -t $TAG -f scripts/Dockerfile --platform linux/amd64 .

cd $CURRENT_DIR
mkdir -p $BUILD_DIR
cp $INPUT $BUILD_DIR/mqjs_input.js

docker run --rm -v "$BUILD_DIR":/build $TAG bash -c "cd /build && /app/mqjs_compile mqjs_input.js mqjs_output.bin"

cd $CURRENT_DIR
mv $BUILD_DIR/mqjs_output.bin $OUTPUT

# rm_if_exists() {
#   if [ -f "$1" ]; then
#     rm "$1"
#   fi
# }

# clean() {
#   rm_if_exists mqjs_compile
#   rm_if_exists mqjs_compile.o
#   rm_if_exists mqjs_compile.d
#   # rm_if_exists mqjs_compile_stdlib.h
# }

# clean

# CONFIG_X86_32=1 CONFIG_SOFTFLOAT=1 make mqjs_stdlib.h
# mv mqjs_stdlib.h mqjs_compile_stdlib.h

# CONFIG_X86_32=1 CONFIG_SOFTFLOAT=1 make mqjs_compile
# cd $CURRENT_DIR
# $SCRIPT_DIR/../mqjs_compile "$@"

# cd $SCRIPT_DIR/.. # mquickjs directory
# clean
# cd $CURRENT_DIR
