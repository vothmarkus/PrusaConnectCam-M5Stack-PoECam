"""Export flashable images and hashes; never flash hardware."""
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess

repo = Path(__file__).resolve().parent.parent
sketch = "ESP32_PrusaConnectCam_web"
build = repo / ".build/firmware"
# Preserve existing download paths.
output = repo / sketch / "build/m5stack.esp32.m5stack_poe_cam"
output.mkdir(parents=True, exist_ok=True)
partitions = (build / f"{sketch}.ino.partitions.bin").read_bytes()
if hashlib.sha256(partitions).hexdigest() != "148b959cbff1c38aa8e1d5c0ba9d612c54997b945e56a63f41223eef650653a1":
    raise SystemExit("Unexpected partition layout: review update compatibility before publishing")
images = {}
for suffix, offset in [("bin", "0x10000"), ("bootloader.bin", "0x1000"),
                       ("partitions.bin", "0x8000"), ("merged.bin", "0x0")]:
    name = f"{sketch}.ino.{suffix}"
    source = build / name
    if not source.is_file():
        raise SystemExit(f"Build output missing: {source}")
    shutil.copyfile(source, output / name)
    images[name] = {"offset": offset, "bytes": source.stat().st_size,
                   "sha256": hashlib.sha256(source.read_bytes()).hexdigest()}
version = re.search(r'#define SW_VERSION\s+"([^"]+)"', (repo / sketch / "mcu_cfg.h").read_text()).group(1)
cli_version = subprocess.check_output([os.environ.get("ARDUINO_CLI", "arduino-cli"), "version"], text=True).strip()
manifest = {"firmware_version": version, "arduino_cli": cli_version,
            "core": "esp32:esp32@3.2.0", "board": "esp32:esp32:m5stack_poe_cam",
            "settings": {"psram": "enabled", "partition_scheme": "default", "flash_mode": "dio",
                         "flash_frequency_mhz": 80, "flash_layout_mb": 4, "cpu_mhz": 240},
            "images": images}
(output / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
(output / "SHA256SUMS").write_text("".join(f"{v['sha256']}  {k}\n" for k, v in images.items()))
print(f"Firmware {version} exported to {output}")
