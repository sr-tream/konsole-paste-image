#!/usr/bin/env bash
set -euo pipefail

HERE="$(cd "$(dirname "$0")" && pwd)"
SRC_DIR="${HERE}/konsole-src"
BUILD_DIR="${HERE}/build"

KONSOLE_VER="$(pacman -Q konsole | awk '{print $2}' | cut -d- -f1)"
echo "Installed konsole: ${KONSOLE_VER}"

if [[ ! -d "${SRC_DIR}" ]]; then
    git clone --depth=1 --branch "v${KONSOLE_VER}" \
        https://invent.kde.org/utilities/konsole.git "${SRC_DIR}"
fi

# Konsole's PluginManager silently rejects plugins whose Version field's first
# 5 chars don't match Konsole's RELEASE_SERVICE_VERSION. Keep them in sync.
sed -i "s/\"Version\": \".*\"/\"Version\": \"${KONSOLE_VER}\"/" \
    "${HERE}/konsole_pasteimageplugin.json"

cmake -S "${HERE}" -B "${BUILD_DIR}" \
    -DCMAKE_BUILD_TYPE=Release \
    -DKONSOLE_SOURCE_DIR="${SRC_DIR}"

cmake --build "${BUILD_DIR}" -j"$(nproc)"

echo
echo "Built: ${BUILD_DIR}/konsole_pasteimageplugin.so"
echo "Install with:"
echo "  pkexec cmake --install ${BUILD_DIR}"
echo "Then fully quit and restart Konsole."
