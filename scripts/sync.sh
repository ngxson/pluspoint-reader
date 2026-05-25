#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$SCRIPT_DIR/.."
UPSTREAM="$ROOT/crosspoint-reader"

cp -r "$UPSTREAM/lib" "$ROOT/"

mkdir -p "$ROOT/lib/vendor"
find "$UPSTREAM/open-x4-sdk" \( -name "*.cpp" -o -name "*.h" \) -exec cp {} "$ROOT/lib/vendor/" \;
