#!/bin/bash
set -e

# Source Emscripten environment
# IMPORTANT: Change this path to match your emsdk installation location
source "$HOME/dev/emsdk/emsdk_env.sh"

# Paths - automatically detect script location
MMAPPER_SRC="$(cd "$(dirname "$0")" && pwd)"
QT_WASM="$MMAPPER_SRC/6.5.3/wasm_multithread"
QT_HOST="$MMAPPER_SRC/6.5.3/macos"

# Configure with qt-cmake
"$QT_WASM/bin/qt-cmake" \
    -S "$MMAPPER_SRC" \
    -B "$MMAPPER_SRC/build-wasm" \
    -DQT_HOST_PATH="$QT_HOST" \
    -DWITH_OPENSSL=OFF \
    -DWITH_TESTS=OFF \
    -DWITH_WEBSOCKET=ON \
    -DWITH_UPDATER=OFF \
    -DCMAKE_BUILD_TYPE=Release

# Build (limited to 4 cores to avoid system slowdown)
cmake --build "$MMAPPER_SRC/build-wasm" --parallel 4

echo ""
echo "Build complete! Run: cd build-wasm/src && python3 ../../server.py"
echo "Then open: http://localhost:9742/mmapper.html"
