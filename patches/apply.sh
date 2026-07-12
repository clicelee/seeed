#!/usr/bin/env bash
# Apply our firmware changes onto a fresh meshtastic/firmware clone.
# Usage: ./patches/apply.sh [path-to-firmware]   (default: ./meshtastic-firmware)
set -euo pipefail
FW="${1:-$(dirname "$0")/../meshtastic-firmware}"

[ -d "$FW/src" ] || { echo "firmware tree not found at $FW"; exit 1; }

for p in "$(dirname "$0")"/*.patch; do
  echo "applying $(basename "$p")"
  git -C "$FW" apply --3way "$p" || git -C "$FW" apply "$p"
done
echo "done. build with: pio run -e seeed_xiao_nrf52840_kit_i2c"
