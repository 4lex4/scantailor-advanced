#!/usr/bin/env bash
# Cross-compile ScanTailor Advanced to Windows .exe from Linux using MXE.
#
# Prerequisites:
#   1. Install MXE (M Cross Environment): https://mxe.cc/
#   2. Build the required packages (one-time, can take 1–2 hours):
#      cd /path/to/mxe
#      make MXE_TARGETS=x86_64-w64-mingw32.static \
#        qt5 jpeg libpng tiff zlib boost
#
# Usage:
#   MXE_DIR=/path/to/mxe ./build-windows.sh
#   ./build-windows.sh                    # uses MXE_DIR or ~/mxe or /opt/mxe
#   ./build-windows.sh shared             # use .shared target (exe + DLLs)
#
# Output: build-win-static/scantailor.exe (default), or build-win-shared/ for shared (exe + DLLs)

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# Target: static (single exe) or shared (exe + DLLs)
SUFFIX="${1:-static}"
if [[ "$SUFFIX" != "static" && "$SUFFIX" != "shared" ]]; then
  echo "Usage: $0 [static|shared]" >&2
  echo "  static = one .exe (default); shared = exe + DLLs" >&2
  exit 1
fi

MXE_TARGET="x86_64-w64-mingw32.${SUFFIX}"
BUILD_DIR="build-win-${SUFFIX}"

# Find MXE root
if [[ -n "$MXE_DIR" && -d "$MXE_DIR" ]]; then
  MXE_ROOT="$MXE_DIR"
elif [[ -d "$HOME/mxe" ]]; then
  MXE_ROOT="$HOME/mxe"
elif [[ -d "/opt/mxe" ]]; then
  MXE_ROOT="/opt/mxe"
else
  echo "Error: MXE not found. Set MXE_DIR or install MXE in ~/mxe or /opt/mxe." >&2
  echo "" >&2
  echo "One-time setup:" >&2
  echo "  git clone https://github.com/mxe/mxe.git ~/mxe" >&2
  echo "  cd ~/mxe && make MXE_TARGETS=${MXE_TARGET} qt5 jpeg libpng tiff zlib boost" >&2
  echo "  (This can take 1–2 hours.)" >&2
  exit 1
fi

TOOLCHAIN_FILE="${MXE_ROOT}/usr/${MXE_TARGET}/share/cmake/mxe-conf.cmake"
if [[ ! -f "$TOOLCHAIN_FILE" ]]; then
  echo "Error: MXE toolchain not found: $TOOLCHAIN_FILE" >&2
  echo "Build the MXE target first:" >&2
  echo "  cd $MXE_ROOT && make MXE_TARGETS=${MXE_TARGET} qt5 jpeg libpng tiff zlib boost" >&2
  exit 1
fi

# Use a dedicated build dir so CMake does not pick up ../libs (native Windows deps).
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

echo "Cross-compiling for Windows (${MXE_TARGET})..."
cmake -DCMAKE_TOOLCHAIN_FILE="${TOOLCHAIN_FILE}" \
  -DCMAKE_BUILD_TYPE=Release \
  ..

make -j"$(nproc)"

if [[ -f scantailor.exe ]]; then
  EXE_PATH="scantailor.exe"
elif [[ -f release/scantailor.exe ]]; then
  EXE_PATH="release/scantailor.exe"
else
  EXE_PATH=$(find . -name "scantailor.exe" -type f 2>/dev/null | head -1)
fi

if [[ -z "$EXE_PATH" || ! -f "$EXE_PATH" ]]; then
  echo "Error: scantailor.exe not found in $BUILD_DIR" >&2
  exit 1
fi

echo ""
echo "Done. Windows executable: $(cd "$SCRIPT_DIR" && realpath "$BUILD_DIR/$EXE_PATH")"
if [[ "$SUFFIX" == "shared" ]]; then
  echo "For .shared build, copy the required DLLs from ${MXE_ROOT}/usr/${MXE_TARGET}/bin/ next to the .exe when deploying."
fi
