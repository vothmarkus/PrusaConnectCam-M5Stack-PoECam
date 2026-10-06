"""Export flashable images and hashes; never flash hardware."""
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
from artifact_meta import ota_manifest, source_digest

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
if len(app) > 0x140000:
    raise SystemExit("Firmware exceeds the 1280 KiB OTA slot")
link_map = (build / f"{sketch}.ino.map").read_text()
if not re.search(r"^verifyRollbackLater\s+.*[/\\]sketch[/\\]ota.cpp.o$", link_map, flags=re.M):
    raise SystemExit("Arduino rollback hook does not resolve to ota.cpp; automatic boot confirmation would defeat self-test")
bootloader = (build / f"{sketch}.ino.bootloader.bin").read_bytes()
if hashlib.sha256(bootloader).hexdigest() != "a284abdd0e4339f9ae18030953606a21d9d2f86d41d78ac46d606226f03ba939":
    raise SystemExit("Bootloader differs from the original rollback-capable M5Stack bootloader")
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
# USB recovery selects app0 after writing it, even when OTA last selected app1.
# This is exactly the two otadata sectors; the preceding NVS is untouched.
reset = bytes([255]) * 0x2000
(output / "ota-reset.bin").write_bytes(reset)
images["ota-reset.bin"] = {"offset": "0xe000", "bytes": len(reset), "sha256": hashlib.sha256(reset).hexdigest()}
cli_version = subprocess.check_output([os.environ.get("ARDUINO_CLI", "arduino-cli"), "version"], text=True).strip()
manifest = {"firmware_version": version, "source_sha256": source_digest(repo), "arduino_cli": cli_version,
            "core": "m5stack:esp32@3.2.2", "board": "m5stack:esp32:m5stack_poe_cam",
            "arduino_core_version": "3.2.1", "esp_idf": idf_version, "fqbn": fqbn,
            "settings": {"psram": "enabled", "partition_scheme": "default", "flash_mode": "qio",
                         "flash_frequency_mhz": 80, "flash_layout_mb": 4, "cpu_mhz": 240},
            "ota": {"target": "m5stack-poe-cam-u121", "layout": "default-4m-ota-1280k-v1",
                    "slot_bytes": 0x140000, "bootloader_rollback": True, "boot_test_override": True},
            "images": images}
(output / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
(output / "ota-manifest.json").write_text(json.dumps(ota_manifest(version, app), indent=2) + "\n")
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
for kind, name in [("bootloader", f"{sketch}.ino.bootloader.bin"), ("otaReset", "ota-reset.bin")]:
    info = images[name]
    web_images[kind] = {"file": name, "address": int(info["offset"], 16), "bytes": info["bytes"],
                        "sha256": info["sha256"], "eraseAll": False}
block = "  const FW = " + json.dumps(web_images, indent=2).replace("\n", "\n  ") + ";"
flasher, count = re.subn(r"  const FW = \{.*?\n  \};", lambda _: block, flasher, flags=re.S)
if count != 1:
    raise SystemExit("Cannot update browser flasher image metadata")
flasher, count = re.subn(r'(data-firmware-version>)[^<]+', lambda m: m[1] + version, flasher)
if count != 3:
    raise SystemExit("Cannot update browser flasher version labels")
flasher_path.write_text(flasher)
print(f"Firmware {version} exported to {output}")
