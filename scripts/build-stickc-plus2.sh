#!/usr/bin/env bash
# Build the M5StickC Plus2 flash image into build/M5Nemo-M5StickCPlus2.bin.
#
# M5Unified 0.2.8 must stay on M5GFX 0.2.36. Leaving M5GFX unpinned installs
# 0.3.x, and that combination boots this board with a black screen.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

M5GFX_VERSION="0.2.36"
M5UNIFIED_VERSION="0.2.8"
CORE_VERSION="3.2.5"
INDEX_URL="https://m5stack.oss-cn-shenzhen.aliyuncs.com/resource/arduino/package_m5stack_index.json"
FQBN="m5stack:esp32:m5stack_stickc_plus2"

SKETCHBOOK="$ROOT/.arduino/sketchbook"
CONFIG="$ROOT/.arduino/cli.yaml"
OUT_DIR="$ROOT/build/stickc-plus2"
MERGED="$ROOT/build/M5Nemo-M5StickCPlus2.bin"

if [[ -n "${ARDUINO_DATA:-}" ]]; then
  DATA="$ARDUINO_DATA"
elif [[ -d "$HOME/Library/Arduino15" ]]; then
  DATA="$HOME/Library/Arduino15"
else
  DATA="$HOME/.arduino15"
fi

if [[ -n "${ARDUINO_CLI:-}" && -x "${ARDUINO_CLI}" ]]; then
  CLI="$ARDUINO_CLI"
elif command -v arduino-cli >/dev/null 2>&1; then
  CLI="$(command -v arduino-cli)"
elif [[ -x "/Applications/Arduino IDE.app/Contents/Resources/app/lib/backend/resources/arduino-cli" ]]; then
  CLI="/Applications/Arduino IDE.app/Contents/Resources/app/lib/backend/resources/arduino-cli"
else
  echo "arduino-cli not found. Install it or set ARDUINO_CLI." >&2
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

mkdir -p "$SKETCHBOOK" "$OUT_DIR" "$ROOT/build"
cat > "$CONFIG" <<EOF
directories:
  user: ${SKETCHBOOK}
  data: ${DATA}
EOF

lib_version() {
  local props="$SKETCHBOOK/libraries/$1/library.properties"
  if [[ ! -f "$props" ]]; then
    return 0
  fi
  awk -F= '/^version=/{print $2; exit}' "$props"
}

echo "Arduino CLI: $CLI"
echo "Sketchbook:  $SKETCHBOOK"

if ! "$CLI" --config-file "$CONFIG" core list | grep -q "m5stack:esp32"; then
  echo "Installing m5stack:esp32@${CORE_VERSION}"
  "$CLI" --config-file "$CONFIG" core update-index --additional-urls "$INDEX_URL"
  "$CLI" --config-file "$CONFIG" core install "m5stack:esp32@${CORE_VERSION}" --additional-urls "$INDEX_URL"
fi

if [[ "$(lib_version M5GFX)" != "$M5GFX_VERSION" || "$(lib_version M5Unified)" != "$M5UNIFIED_VERSION" || ! -d "$SKETCHBOOK/libraries/IRremote" ]]; then
  echo "Installing libraries and pinning M5GFX ${M5GFX_VERSION} and M5Unified ${M5UNIFIED_VERSION}"
  "$CLI" --config-file "$CONFIG" lib install IRremote "M5Stack-SD-Updater" "M5StickCPlus2@1.0.1"
  "$CLI" --config-file "$CONFIG" lib uninstall M5Unified M5GFX || true
  "$CLI" --config-file "$CONFIG" lib install --no-deps "M5GFX@${M5GFX_VERSION}" "M5Unified@${M5UNIFIED_VERSION}"
fi

gfx="$(lib_version M5GFX)"
unified="$(lib_version M5Unified)"
if [[ "$gfx" != "$M5GFX_VERSION" || "$unified" != "$M5UNIFIED_VERSION" ]]; then
  echo "Library pin failed: M5GFX=${gfx:-missing} M5Unified=${unified:-missing}" >&2
  exit 1
fi
echo "Using M5GFX ${gfx}, M5Unified ${unified}"

log="$OUT_DIR/compile.log"
for attempt in 1 2 3 4 5 6; do
  echo "Compile attempt ${attempt}"
  set +e
  "$CLI" --config-file "$CONFIG" compile \
    --fqbn "$FQBN" \
    --jobs 1 \
    --build-property build.partitions=huge_app \
    --build-property upload.maximum_size=3145728 \
    --build-property "compiler.cpp.extra_flags=-DLANGUAGE_EN_US" \
    --output-dir "$OUT_DIR" \
    "$ROOT/src/m5stick-nemo" >"$log" 2>&1
  code=$?
  set -e
  if [[ "$code" -eq 0 ]]; then
    break
  fi
  missing="$(python3 - "$log" <<'PY'
import re
import sys
from pathlib import Path
text = Path(sys.argv[1]).read_text(errors="replace")
match = re.search(r"opening dependency file ([^:]+): No such file", text)
print(Path(match.group(1)).parent if match else "")
PY
)"
  if [[ -z "$missing" ]]; then
    echo "Compile failed. See ${log}" >&2
    grep -n -E "error:|overflowed" "$log" | head -30 >&2 || true
    exit 1
  fi
  mkdir -p "$missing"
  echo "Created missing dependency directory ${missing}"
  if [[ "$attempt" -eq 6 ]]; then
    echo "Compile failed after creating dependency directories. See ${log}" >&2
    exit 1
  fi
done

if [[ ! -f "$OUT_DIR/m5stick-nemo.ino.bin" ]]; then
  echo "Compile did not produce the application binary." >&2
  exit 1
fi

# Capture the version first. Piping into grep -q under pipefail treats
# esptool's SIGPIPE as a failure and would pick the old merge command.
esptool_version="$("${ESPTOOL[@]}" version 2>&1 || true)"
if [[ "$esptool_version" == *5.* ]]; then
  "${ESPTOOL[@]}" --chip esp32 merge-bin -o "$MERGED" \
    0x1000 "$OUT_DIR/m5stick-nemo.ino.bootloader.bin" \
    0x8000 "$OUT_DIR/m5stick-nemo.ino.partitions.bin" \
    0x10000 "$OUT_DIR/m5stick-nemo.ino.bin"
else
  "${ESPTOOL[@]}" --chip esp32 merge_bin -o "$MERGED" \
    0x1000 "$OUT_DIR/m5stick-nemo.ino.bootloader.bin" \
    0x8000 "$OUT_DIR/m5stick-nemo.ino.partitions.bin" \
    0x10000 "$OUT_DIR/m5stick-nemo.ino.bin"
fi

ls -lh "$MERGED"
echo
echo "Flash with:"
echo "esptool --chip esp32 --port <porta> write-flash -z 0x0 build/M5Nemo-M5StickCPlus2.bin"
