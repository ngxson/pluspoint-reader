#!/bin/bash

set -e

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

cd $SCRIPT_DIR/.. # project root
cd mquickjs

make clean

CONFIG_X86_32=1 CONFIG_SOFTFLOAT=1 make mqjs_stdlib.h
CONFIG_X86_32=1 CONFIG_SOFTFLOAT=1 make mquickjs_atom.h

cp mqjs_stdlib.h ../lib/mquickjs/mqjs_stdlib.h
cp mquickjs_atom.h ../lib/mquickjs/mquickjs_atom.h

make clean
# git reset --hard
