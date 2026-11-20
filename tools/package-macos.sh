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

# The artefact upload either keeps the name of the .vst3 folder or strips it
# away, depending on how the bundle was handed over. Accept both shapes, so
# this script works with an old artefact and with a new one.
bundle_root () {
    local dir="$1"

    if [ -d "$dir/$PROJECT.vst3/Contents" ]; then
        printf '%s\n' "$dir/$PROJECT.vst3"
        return 0
    fi

    if [ -d "$dir/Contents" ]; then
        printf '%s\n' "$dir"
        return 0
    fi

    return 1
}

X86_BUNDLE="$(bundle_root "$RAW/bundle-$PROJECT-x86_64" || true)"
ARM_BUNDLE="$(bundle_root "$RAW/bundle-$PROJECT-arm64" || true)"

for bundle in "$X86_BUNDLE" "$ARM_BUNDLE"; do
    if [ -z "$bundle" ]; then
        echo "ERROR: missing the bundle for $PROJECT (looked in $RAW/bundle-$PROJECT-*)" >&2
        exit 1
    fi
done

rm -rf "$STAGE"
mkdir -p "$STAGE"
ditto "$X86_BUNDLE" "$STAGE/$PROJECT.vst3"

# The binary inside a macOS VST3 bundle lives in Contents/MacOS and carries the
# plugin name without the .vst3 extension (PIER-MODE, not PIER-MODE.vst3).
plugin_binary () {
    find "$1/Contents/MacOS" -maxdepth 1 -type f -print -quit
}

X86_BIN="$(plugin_binary "$X86_BUNDLE")"
ARM_BIN="$(plugin_binary "$ARM_BUNDLE")"
STAGE_BIN="$(plugin_binary "$STAGE/$PROJECT.vst3")"

if [ -z "$X86_BIN" ] || [ -z "$ARM_BIN" ] || [ -z "$STAGE_BIN" ]; then
    echo "ERROR: cannot find the plugin binary inside the bundle" >&2
    exit 1
fi

echo "--- sources ---"
echo "x86_64: $X86_BIN"
echo "arm64:  $ARM_BIN"

lipo -create "$X86_BIN" "$ARM_BIN" -output "$STAGE_BIN"

echo "--- architectures ---"
lipo -info "$STAGE_BIN"

# The per architecture signature is meaningless once the binary is glued
# together: drop it and sign the finished bundle again.
rm -rf "$STAGE/$PROJECT.vst3/Contents/_CodeSignature"
codesign --force --deep --sign - "$STAGE/$PROJECT.vst3"

# The folder where the user presets live. It is not always the plugin name:
# the two plugins with a longer name use a shorter folder, so that the path
# stays readable.
case "$PROJECT" in
    EchoMatrixMode)        PRESET_FOLDER="ECHOMATRIX-MODE" ;;
    HarmonicResonatorMode) PRESET_FOLDER="HARMONIC-RESONATOR-MODE" ;;
    MemoryGranulatorMode)  PRESET_FOLDER="MEMORY-GRANULATOR-MODE" ;;
    *)                     PRESET_FOLDER="$PROJECT" ;;
esac

# The friend facing instructions, with the plugin name filled in.
sed -e "s/PLUGIN_NAME/$PROJECT/g" -e "s/PRESET_FOLDER/$PRESET_FOLDER/g" \
    "$ROOT/tools/MAC-INSTALL.md" \
    > "$STAGE/LEGGIMI - installazione macOS.md"

# Keep the plugin-specific guide in the archive as well. For instruments with
# a MIDI bus this is where Ableton routing, first steps and troubleshooting live.
PLUGIN_GUIDE="LEGGIMI - istruzioni $PROJECT.md"
if [ -f "$ROOT/$PROJECT/INSTALLAZIONE.md" ]; then
    cp "$ROOT/$PROJECT/INSTALLAZIONE.md" "$STAGE/$PLUGIN_GUIDE"
fi

# The licence text travels with the binary, as the AGPLv3 asks for.
cp "$ROOT/LICENSE.md" "$STAGE/LICENZA - AGPLv3.md"

mkdir -p "$OUT"
# An absolute path, because the zip is made from inside the staging folder.
OUT="$(cd "$OUT" && pwd)"
rm -f "$OUT/$PROJECT-macOS-VST3.zip"

archive_items=(
    "$PROJECT.vst3"
    "LEGGIMI - installazione macOS.md"
    "LICENZA - AGPLv3.md"
)

if [ -f "$STAGE/$PLUGIN_GUIDE" ]; then
    archive_items+=("$PLUGIN_GUIDE")
fi

( cd "$STAGE" && zip -r -y "$OUT/$PROJECT-macOS-VST3.zip" \
    "${archive_items[@]}" > /dev/null )

echo ">>> $OUT/$PROJECT-macOS-VST3.zip"
