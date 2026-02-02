#!/bin/bash

set -e

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

cd $SCRIPT_DIR/.. # mquickjs directory

make clean

CONFIG_X86_32=1 CONFIG_SOFTFLOAT=1 make mqjs_stdlib.h
CONFIG_X86_32=1 CONFIG_SOFTFLOAT=1 make mquickjs_atom.h

mv mqjs_stdlib.h crosspoint_stdlib.h

mv mquickjs_atom.h crosspoint_atom.h # avoid being deleted in the next step
make clean
mv crosspoint_atom.h mquickjs_atom.h
