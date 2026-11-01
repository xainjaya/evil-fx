#!/usr/bin/env bash
# Builds "EvilFX-<version>-macOS.pkg" from a Release build: VST3, AU, CLAP and the
# standalone app, each as its own optional component.
#
#   packaging/macos/make_pkg.sh <version> [artefacts dir]
#
# Signing + notarization happen automatically when these are set (CI reads them
# from repository secrets; locally, export them first):
#   MACOS_DEV_ID_APP        "Developer ID Application: Your Name (TEAMID)"
#   MACOS_DEV_ID_INSTALLER  "Developer ID Installer: Your Name (TEAMID)"
#   APPLE_ID, APPLE_TEAM_ID, APPLE_APP_PASSWORD   (app-specific password for notarytool)
# Without them you still get a working, unsigned .pkg (Gatekeeper will warn).
set -euo pipefail

VERSION="${1:?usage: make_pkg.sh <version> [artefacts dir]}"
ART="${2:-build/EvilFX_artefacts/Release}"
OUT="dist"
WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT
mkdir -p "$OUT"

sign_bundle() {
  local b="$1"
  [ -n "${MACOS_DEV_ID_APP:-}" ] || return 0
  echo "codesign: $b"
  codesign --force --deep --options runtime --timestamp --sign "$MACOS_DEV_ID_APP" "$b"
  codesign --verify --deep --strict "$b"
}

component() {   # id, source bundle, install location
  local id="$1" src="$2" dest="$3"
  if [ ! -e "$src" ]; then echo "skip $id (no $src)"; return 0; fi
  mkdir -p "$WORK/root-$id"
  cp -R "$src" "$WORK/root-$id/"
  sign_bundle "$WORK/root-$id/$(basename "$src")"
  pkgbuild --root "$WORK/root-$id" --identifier "com.xainjaya.evilfx.$id" --version "$VERSION" \
           --install-location "$dest" "$WORK/$id.pkg"
  CHOICES+=("$id")
}

CHOICES=()
component vst3       "$ART/VST3/Evil FX.vst3"           "/Library/Audio/Plug-Ins/VST3"
component au         "$ART/AU/Evil FX.component"        "/Library/Audio/Plug-Ins/Components"
component clap       "$ART/CLAP/Evil FX.clap"           "/Library/Audio/Plug-Ins/CLAP"
component standalone "$ART/Standalone/Evil FX.app"      "/Applications"

{
  echo '<?xml version="1.0" encoding="utf-8"?>'
  echo '<installer-gui-script minSpecVersion="2">'
  echo "  <title>Evil FX $VERSION</title>"
  echo '  <options customize="always" require-scripts="false" hostArchitectures="x86_64,arm64"/>'
  echo '  <choices-outline>'
  for c in "${CHOICES[@]}"; do echo "    <line choice=\"$c\"/>"; done
  echo '  </choices-outline>'
  for c in "${CHOICES[@]}"; do
    case "$c" in vst3) t="VST3 plugin";; au) t="Audio Unit (Logic, GarageBand)";; clap) t="CLAP plugin";; *) t="Standalone app";; esac
    echo "  <choice id=\"$c\" title=\"$t\"><pkg-ref id=\"com.xainjaya.evilfx.$c\"/></choice>"
    echo "  <pkg-ref id=\"com.xainjaya.evilfx.$c\">$c.pkg</pkg-ref>"
  done
  echo '</installer-gui-script>'
} > "$WORK/distribution.xml"

PKG="$OUT/EvilFX-$VERSION-macOS.pkg"
if [ -n "${MACOS_DEV_ID_INSTALLER:-}" ]; then
  productbuild --distribution "$WORK/distribution.xml" --package-path "$WORK" --sign "$MACOS_DEV_ID_INSTALLER" "$PKG"
else
  productbuild --distribution "$WORK/distribution.xml" --package-path "$WORK" "$PKG"
fi

if [ -n "${APPLE_ID:-}" ] && [ -n "${APPLE_TEAM_ID:-}" ] && [ -n "${APPLE_APP_PASSWORD:-}" ] && [ -n "${MACOS_DEV_ID_INSTALLER:-}" ]; then
  xcrun notarytool submit "$PKG" --apple-id "$APPLE_ID" --team-id "$APPLE_TEAM_ID" --password "$APPLE_APP_PASSWORD" --wait
  xcrun stapler staple "$PKG"
  echo "notarized: $PKG"
else
  echo "built UNSIGNED: $PKG (set the signing variables to sign + notarize)"
fi
