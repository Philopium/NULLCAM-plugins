#!/usr/bin/env bash
# Builds one NULLCAM plugin for macOS, one architecture.
#
#   tools/build-macos.sh <project-dir> <cmake-target> <arch>
#
# The finished bundle is copied to out/<project>.vst3 so the workflow can pick
# it up and glue it together with the other architecture.
set -euo pipefail

PROJECT="${1:?project directory}"
TARGET="${2:?cmake target}"
ARCH="${3:?architecture}"

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD="$ROOT/build/$PROJECT-$ARCH"
OUT="$ROOT/out"
DEPLOYMENT_TARGET="${DEPLOYMENT_TARGET:-11.0}"

cd "$ROOT"

echo "============================================"
echo " $PROJECT  ->  $TARGET  ($ARCH)"
echo "============================================"

CMAKE_OSX_DEPLOYMENT_TARGET="$DEPLOYMENT_TARGET" \
cmake -S "$PROJECT" -B "$BUILD" -G Xcode \
      -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_OSX_ARCHITECTURES="$ARCH" \
      -DCMAKE_OSX_DEPLOYMENT_TARGET="$DEPLOYMENT_TARGET"

cmake --build "$BUILD" --config Release --target "$TARGET" --parallel

# Locate the freshly built bundle and copy it out under a stable name.
VST3="$(find "$BUILD" -maxdepth 8 -type d -name '*.vst3' -path '*VST3*' | head -1)"

if [ -z "$VST3" ]; then
    echo "ERROR: no .vst3 bundle found under $BUILD" >&2
    exit 1
fi

mkdir -p "$OUT"
rm -rf "$OUT/$PROJECT.vst3"
ditto "$VST3" "$OUT/$PROJECT.vst3"

# Ad-hoc signature. It is not a Developer ID signature, but it stops macOS from
# complaining that the bundle has never been signed at all.
if command -v codesign >/dev/null 2>&1; then
    codesign --force --deep --sign - "$OUT/$PROJECT.vst3" || true
fi

echo ">>> built $OUT/$PROJECT.vst3"
