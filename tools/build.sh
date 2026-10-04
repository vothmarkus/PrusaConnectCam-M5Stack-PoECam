#!/usr/bin/env bash
set -euo pipefail
repo=$(cd "$(dirname "$0")/.." && pwd)
cli=${ARDUINO_CLI:-arduino-cli}
"$cli" core list --format json | python3 -c '
import json, sys
platforms = json.load(sys.stdin).get("platforms", [])
if not any(p.get("id") == "m5stack:esp32" and p.get("installed_version") == "3.2.2" for p in platforms):
    sys.exit("Install the original board package: arduino-cli core install m5stack:esp32@3.2.2 --additional-urls https://static-cdn.m5stack.com/resource/arduino/package_m5stack_index.json")
'
# Match the working pre-1.3.0 ELF, including its SDK and QIO bootloader.
fqbn='m5stack:esp32:m5stack_poe_cam:UploadSpeed=1500000,CPUFreq=240,FlashFreq=80,FlashMode=qio,FlashSize=4M,PartitionScheme=default,DebugLevel=none,PSRAM=enabled,LoopCore=1,EventsCore=1,EraseFlash=none'
"$cli" compile --fqbn "$fqbn" --warnings all --build-path "$repo/.build/firmware" "$repo/ESP32_PrusaConnectCam_web"
python3 "$repo/tools/package_firmware.py"
