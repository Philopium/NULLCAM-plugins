#!/usr/bin/env bash
# Works out which plugins are ready to be built and prints the build matrix.
#
#   line 1: {"include":[ ... ]}   -> strategy.matrix
#   line 2: ["PIER-MODE", ...]    -> the list of projects to package
#
# A project counts as ready when it has a CMakeLists.txt and a plugin
# processor, so a half finished plugin never breaks the build.
set -euo pipefail

cd "$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

matrix_entries=""
projects=""
sep_matrix=""
sep_projects=""

for dir in PIER-MODE MAURI-MODE SPAMPI-MODE GIANNIS-MODE EchoMatrixMode HarmonicResonatorMode MemoryGranulatorMode; do
    if [ ! -f "$dir/CMakeLists.txt" ] || [ ! -f "$dir/Source/PluginProcessor.cpp" ]; then
        echo "::notice::skipping $dir (not ready yet)" >&2
        continue
    fi

    target="$(sed -n 's/^juce_add_plugin(\([A-Za-z0-9_]*\).*/\1/p' "$dir/CMakeLists.txt" | head -1)"

    if [ -z "$target" ]; then
        echo "::error::cannot find the juce_add_plugin target in $dir/CMakeLists.txt" >&2
        continue
    fi

    # macOS 14 images were retired by GitHub on 2 November 2026, and the
    # macOS 13 ones before that. These are the labels that exist today:
    #   macos-15-intel -> Intel Mac (x86_64)
    #   macos-15       -> Apple Silicon (arm64)
    for spec in "macos-15-intel x86_64" "macos-15 arm64"; do
        # shellcheck disable=SC2086
        set -- $spec
        matrix_entries="${matrix_entries}${sep_matrix}{project:\"$dir\",target:\"${target}_VST3\",runner:\"$1\",arch:\"$2\"}"
        sep_matrix=","
    done

    projects="${projects}${sep_projects}\"$dir\""
    sep_projects=","
done

echo "{\"include\":[${matrix_entries}]}"
echo "[${projects}]"
