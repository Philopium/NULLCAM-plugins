#!/usr/bin/env bash
# Builds the VST3 for macOS (universal) and packs the zip to send to someone.
set -euo pipefail
cd "$(dirname "$0")"

echo "============================================"
echo " PIER-MODE - build VST3 per macOS"
echo "============================================"

if ! command -v cmake >/dev/null 2>&1; then
    echo "ERRORE: cmake non trovato. Installa con: brew install cmake"
    exit 1
fi
if ! xcode-select -p >/dev/null 2>&1; then
    echo "ERRORE: mancano i Command Line Tools. Installa con: xcode-select --install"
    exit 1
fi

echo "[1/3] Configurazione..."
cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"

echo "[2/3] Compilazione (la prima volta scarica anche JUCE, ci vuole qualche minuto)..."
cmake --build build --config Release --parallel

SRC="build/NullcamChain_artefacts/Release/VST3/PIER-MODE.vst3"
if [ ! -d "$SRC" ]; then
    echo "ERRORE: non trovo $SRC"
    exit 1
fi

echo "[3/3] Preparo lo zip da mandare..."
rm -rf dist
mkdir -p dist
ditto "$SRC" "dist/PIER-MODE.vst3"
cp INSTALLAZIONE.md "dist/LEGGIMI - installazione.md"

rm -f PIER-MODE-macOS-VST3.zip
( cd dist && ditto -c -k --sequesterRsrc --keepParent "PIER-MODE.vst3" \
    "../PIER-MODE-macOS-VST3.zip" \
    && zip -g -y "../PIER-MODE-macOS-VST3.zip" "LEGGIMI - installazione.md" >/dev/null )

echo
echo "============================================"
echo " Fatto"
echo "============================================"
echo " Plugin: $PWD/$SRC"
echo " Zip:    $PWD/PIER-MODE-macOS-VST3.zip"
echo
echo " Il plugin e' gia' in ~/Library/Audio/Plug-Ins/VST3."
echo " In Ableton: Impostazioni -> Plug-In -> Riscansiona, cerca \"PIER-MODE\"."
