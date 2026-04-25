#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="${1:-$ROOT_DIR/build}"
APPDIR="${2:-$BUILD_DIR/AppDir}"

if ! command -v ./linuxdeploy*.AppImage >/dev/null 2>&1; then
  echo "linuxdeploy is required or was not found in $(pwd). Install it and re-run this script." >&2
  exit 1
fi

cmake --install "$BUILD_DIR" --prefix "$APPDIR/usr"

DESKTOP_FILE="$APPDIR/usr/share/applications/audiovis.desktop"
ICON_FILE="$APPDIR/usr/share/icons/hicolor/512x512/apps/audiovis.png"
EXECUTABLE="$APPDIR/usr/bin/audiovis"
LIBSCROB="$APPDIR/usr/lib/libscrob.so"

if [[ ! -f "$DESKTOP_FILE" ]]; then
  echo "Missing desktop file at $DESKTOP_FILE" >&2
  exit 1
fi

if [[ ! -f "$ICON_FILE" ]]; then
  echo "Missing icon file at $ICON_FILE" >&2
  exit 1
fi

if [[ ! -x "$EXECUTABLE" ]]; then
  echo "Missing executable at $EXECUTABLE" >&2
  exit 1
fi

if [[ ! -f "$LIBSCROB" ]]; then
  echo "Missing libscrob shared library at $LIBSCROB" >&2
  exit 1
fi

./linuxdeploy*.AppImage \
  --appdir "$APPDIR" \
  --desktop-file "$DESKTOP_FILE" \
  --icon-file "$ICON_FILE" \
  --executable "$EXECUTABLE" \
  --library "$LIBSCROB" \
  --output appimage

echo "AppImage generated in $BUILD_DIR"
