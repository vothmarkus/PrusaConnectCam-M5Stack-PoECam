#!/usr/bin/env bash
set -euo pipefail
repo=$(cd "$(dirname "$0")/.." && pwd)
cli=${ARDUINO_CLI:-arduino-cli}
"$cli" core list --format json | python3 -c '
import json, sys
platforms = json.load(sys.stdin).get("platforms", [])
if not any(p.get("id") == "esp32:esp32" and p.get("installed_version") == "3.2.0" for p in platforms):
    sys.exit("Install the reference core first: arduino-cli core install esp32:esp32@3.2.0")
'
fqbn='esp32:esp32:m5stack_poe_cam:PSRAM=enabled,PartitionScheme=default,FlashMode=dio,FlashFreq=80,FlashSize=4M,CPUFreq=240,LoopCore=1,EventsCore=1,DebugLevel=none,EraseFlash=none'
"$cli" compile --fqbn "$fqbn" --warnings all --build-path "$repo/.build/firmware" "$repo/ESP32_PrusaConnectCam_web"
python3 "$repo/tools/package_firmware.py"
