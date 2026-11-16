#!/usr/bin/env bash
# Glues the two architecture builds into one universal bundle and packs the zip
# that gets sent to somebody.
#
#   tools/package-macos.sh <project-dir> <raw-folder> <out-folder>
#
# The raw folder is where the workflow dropped the downloaded artefacts, one
# folder per architecture.
set -euo pipefail

PROJECT="${1:?project directory}"
RAW="${2:?folder with the per architecture bundles}"
OUT="${3:?output folder}"

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
STAGE="$ROOT/build/package/$PROJECT"

X86_BUNDLE="$RAW/bundle-$PROJECT-x86_64/$PROJECT.vst3"
ARM_BUNDLE="$RAW/bundle-$PROJECT-arm64/$PROJECT.vst3"

for bundle in "$X86_BUNDLE" "$ARM_BUNDLE"; do
    if [ ! -d "$bundle" ]; then
        echo "ERROR: missing $bundle" >&2
        exit 1
    fi
done

rm -rf "$STAGE"
mkdir -p "$STAGE"
ditto "$X86_BUNDLE" "$STAGE/$PROJECT.vst3"

BIN="$(find "$STAGE/$PROJECT.vst3/Contents" -type f -name "$PROJECT.vst3" | head -1)"

if [ -z "$BIN" ]; then
    echo "ERROR: cannot find the plugin binary inside the bundle" >&2
    exit 1
fi

lipo -create \
    "$X86_BUNDLE/Contents/MacOS/$PROJECT.vst3" \
    "$ARM_BUNDLE/Contents/MacOS/$PROJECT.vst3" \
    -output "$BIN"

echo "--- architectures ---"
lipo -info "$BIN"

codesign --force --deep --sign - "$STAGE/$PROJECT.vst3"

# The friend facing instructions, with the plugin name filled in.
sed "s/PLUGIN_NAME/$PROJECT/g" "$ROOT/tools/MAC-INSTALL.md" \
    > "$STAGE/LEGGIMI - installazione macOS.md"

mkdir -p "$OUT"
rm -f "$OUT/$PROJECT-macOS-VST3.zip"

( cd "$STAGE" && zip -r -y "$OUT/$PROJECT-macOS-VST3.zip" \
    "$PROJECT.vst3" "LEGGIMI - installazione macOS.md" > /dev/null )

echo ">>> $OUT/$PROJECT-macOS-VST3.zip"
