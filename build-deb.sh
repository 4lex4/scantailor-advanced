#!/usr/bin/env bash
# Build ScanTailor Advanced and create a .deb package for Ubuntu/Debian.
# Usage: ./build-deb.sh [build_dir]
# The .deb will be created in the project root.

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# Optional: custom build directory (default: build)
BUILD_DIR="${1:-build}"
mkdir -p "$BUILD_DIR"
BUILD_DIR="$(cd "$BUILD_DIR" && pwd)"
PKG_DIR="${BUILD_DIR}/debian-pkg"
DEBIAN_DIR="${PKG_DIR}/DEBIAN"

# Extract version from version.h.in
VERSION=$(sed -n 's/^#define VERSION "\([^"]*\)".*/\1/p' version.h.in)
if [[ -z "$VERSION" ]]; then
  echo "Error: could not read VERSION from version.h.in" >&2
  exit 1
fi

echo "Building ScanTailor Advanced ${VERSION}"

# Configure and build
cd "$BUILD_DIR"
cmake -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=Release ..
make -j"$(nproc)"

# Install into staging directory for the .deb
rm -rf "$PKG_DIR"
make install DESTDIR="${PKG_DIR}"

# Debian control file
mkdir -p "$DEBIAN_DIR"
ARCH=$(dpkg --print-architecture 2>/dev/null || echo "amd64")

# Generate Depends via dpkg-shlibdeps when available (more accurate)
DEPS=""
if command -v dpkg-shlibdeps >/dev/null 2>&1; then
  SHLIBS_OUT=$(cd "${PKG_DIR}" && dpkg-shlibdeps -e usr/bin/scantailor -O 2>/dev/null || true)
  if [[ -n "$SHLIBS_OUT" ]]; then
    DEPS="${SHLIBS_OUT#shlibs:Depends=}"
  fi
fi
if [[ -z "$DEPS" ]]; then
  DEPS="libc6, libstdc++6, libgcc-s1, libqt5core5t64 | libqt5core5a, libqt5gui5t64 | libqt5gui5, libqt5widgets5t64 | libqt5widgets5, libqt5svg5t64 | libqt5svg5, libqt5xml5t64 | libqt5xml5, libqt5network5t64 | libqt5network5, libboost-filesystem1.83.0 | libboost-filesystem1.74.0, libjpeg62-turbo | libjpeg8, libpng16-16, libtiff6, zlib1g"
fi

cat > "${DEBIAN_DIR}/control" << EOF
Package: scantailor-advanced
Version: ${VERSION}
Section: graphics
Priority: optional
Architecture: ${ARCH}
Depends: ${DEPS}
Maintainer: 4lex4 <4lex49@zoho.com>
Description: Interactive post-processing tool for scanned pages
 ScanTailor Advanced merges features from ScanTailor Featured and Enhanced,
 with improvements for page splitting, deskewing, content selection,
 margins, dewarping and output. Supports batch processing and multiple
 output formats.
Homepage: https://github.com/4lex4/scantailor-advanced
EOF

# Optional: refresh icon and desktop caches after install
cat > "${DEBIAN_DIR}/postinst" << 'POSTINST'
#!/bin/sh
set -e
if command -v update-desktop-database >/dev/null 2>&1; then
  update-desktop-database /usr/share/applications 2>/dev/null || true
fi
if command -v gtk-update-icon-cache >/dev/null 2>&1; then
  gtk-update-icon-cache -f -t /usr/share/icons/hicolor 2>/dev/null || true
fi
if command -v update-mime-database >/dev/null 2>&1; then
  update-mime-database /usr/share/mime 2>/dev/null || true
fi
POSTINST
chmod 755 "${DEBIAN_DIR}/postinst"

# Build the .deb
cd "$SCRIPT_DIR"
dpkg-deb --root-owner-group --build "$PKG_DIR" "scantailor-advanced_${VERSION}_${ARCH}.deb"

echo "Done: scantailor-advanced_${VERSION}_${ARCH}.deb"
