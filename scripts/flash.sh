#!/usr/bin/env bash
# Flash build/M5Nemo-M5StickCPlus2.bin at offset 0x0.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PORT="${1:-}"
IMAGE="$ROOT/build/M5Nemo-M5StickCPlus2.bin"

if [[ -z "$PORT" ]]; then
  echo "Usage: $0 /dev/cu.usbserial-XXXXXXXX" >&2
  exit 1
fi

if [[ ! -f "$IMAGE" ]]; then
  echo "Missing ${IMAGE}. Run ./scripts/build-stickc-plus2.sh first." >&2
  exit 1
fi

if command -v esptool >/dev/null 2>&1; then
  ESPTOOL=(esptool)
elif command -v esptool.py >/dev/null 2>&1; then
  ESPTOOL=(esptool.py)
else
  echo "esptool not found. Install it with: pip install -U esptool" >&2
  exit 1
fi

"${ESPTOOL[@]}" --chip esp32 --port "$PORT" write-flash -z 0x0 "$IMAGE"
