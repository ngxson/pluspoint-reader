#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$SCRIPT_DIR/.."
UPSTREAM="$ROOT/crosspoint-reader"

cp -r "$UPSTREAM/lib" "$ROOT/"
