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
version = re.search(r'#define SW_VERSION\s+"([^"]+)"', (repo / sketch / "mcu_cfg.h").read_text()).group(1)
fqbn = json.loads((build / "build.options.json").read_text())["fqbn"]
if not fqbn.startswith("m5stack:esp32:m5stack_poe_cam:") or "FlashMode=qio" not in fqbn.split(":", 3)[3].split(","):
    raise SystemExit("Build does not use the original M5Stack board / QIO settings")
app = (build / f"{sketch}.ino.bin").read_bytes()
# ESP-IDF app descriptor: first DROM segment starts at byte 32, IDF version at 144.
idf_version = app[144:176].split(b"\0", 1)[0].decode("ascii")
if app[32:36] != bytes.fromhex("3254cdab") or idf_version != "v5.4.2-25-g858a988d6e":
    raise SystemExit("SDK differs from the original working firmware; review before publishing")
if ("M5PoECAM Prusa Connect " + version).encode() not in app:
    raise SystemExit("Firmware version does not match the compiled image; rebuild first")
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
cli_version = subprocess.check_output([os.environ.get("ARDUINO_CLI", "arduino-cli"), "version"], text=True).strip()
manifest = {"firmware_version": version, "arduino_cli": cli_version,
            "core": "m5stack:esp32@3.2.2", "board": "m5stack:esp32:m5stack_poe_cam",
            "arduino_core_version": "3.2.1", "esp_idf": idf_version, "fqbn": fqbn,
            "settings": {"psram": "enabled", "partition_scheme": "default", "flash_mode": "qio",
                         "flash_frequency_mhz": 80, "flash_layout_mb": 4, "cpu_mhz": 240},
            "images": images}
(output / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
(output / "SHA256SUMS").write_text("".join(f"{v['sha256']}  {k}\n" for k, v in images.items()))
# Keep the browser flasher's integrity checks in sync with these exact images.
flasher_path = repo / "docs/index.html"
flasher = flasher_path.read_text()
web_images = {}
for kind, suffix, erase in [("update", "bin", False), ("install", "merged.bin", True)]:
    name = f"{sketch}.ino.{suffix}"
    info = images[name]
    web_images[kind] = {"file": name, "address": int(info["offset"], 16),
                        "bytes": info["bytes"], "sha256": info["sha256"], "eraseAll": erase}
block = "  const FW = " + json.dumps(web_images, indent=2).replace("\n", "\n  ") + ";"
flasher, count = re.subn(r"  const FW = \{.*?\n  \};", lambda _: block, flasher, flags=re.S)
if count != 1:
    raise SystemExit("Cannot update browser flasher image metadata")
flasher, count = re.subn(r'(data-firmware-version>)[^<]+', lambda m: m[1] + version, flasher)
if count != 3:
    raise SystemExit("Cannot update browser flasher version labels")
flasher_path.write_text(flasher)
print(f"Firmware {version} exported to {output}")
