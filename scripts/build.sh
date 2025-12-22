#!/usr/bin/env bash
set -e
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="$ROOT/build"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"
cmake ..
make -j$(nproc)
echo "Build finished. Binaries: $BUILD_DIR/Jumandgi and $BUILD_DIR/create_admin"
