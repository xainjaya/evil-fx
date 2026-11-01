#!/usr/bin/env bash
# Installs Evil FX for the current user from an unpacked Linux build/zip.
#   ./install.sh            (run from the folder containing VST3/, CLAP/, LV2/, Standalone/)
#   ./install.sh --uninstall
set -euo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"
targets=( "$HOME/.vst3/Evil FX.vst3" "$HOME/.clap/Evil FX.clap" "$HOME/.lv2/Evil FX.lv2" "$HOME/.local/bin/evil-fx" )

if [ "${1:-}" = "--uninstall" ]; then
  for t in "${targets[@]}"; do rm -rf "$t" && echo "removed $t"; done
  exit 0
fi

copy() { [ -e "$1" ] || { echo "skip: $1 not found"; return 0; }; mkdir -p "$(dirname "$2")"; rm -rf "$2"; cp -R "$1" "$2"; echo "installed $2"; }
copy "$HERE/VST3/Evil FX.vst3"       "${targets[0]}"
copy "$HERE/CLAP/Evil FX.clap"       "${targets[1]}"
copy "$HERE/LV2/Evil FX.lv2"         "${targets[2]}"
copy "$HERE/Standalone/Evil FX"      "${targets[3]}"
echo "Done. Rescan plugins in your DAW."
