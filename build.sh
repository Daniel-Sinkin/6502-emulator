#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR="build"                
GENERATOR="Unix Makefiles"        
BEEP_OUT="assets/sound/beep.wav"  

echo "Generating CHIP-8 beep…"
mkdir -p "$(dirname "$BEEP_OUT")"
PYTHON_BIN=$(command -v python3 || command -v python)
if [ -z "$PYTHON_BIN" ]; then
  echo " Neither python3 nor python found in PATH."
  exit 1
fi
"$PYTHON_BIN" util/generate_beep.py --output "$BEEP_OUT"

if [ -d "$BUILD_DIR" ]; then
  echo "🧹  Removing old $BUILD_DIR to avoid generator conflicts…"
  rm -rf "$BUILD_DIR"
fi
mkdir -p "$BUILD_DIR"

cmake -S . -B "$BUILD_DIR" \
  -G "$GENERATOR" \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

if command -v sysctl &> /dev/null; then
  JOBS=$(sysctl -n hw.logicalcpu)
elif command -v nproc &> /dev/null; then
  JOBS=$(nproc)
else
  JOBS=1
fi

cmake --build "$BUILD_DIR" --parallel "$JOBS"

echo "Build complete → $BUILD_DIR/main"
