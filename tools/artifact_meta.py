"""Shared firmware artifact invariants, used by packaging and release checks."""
import hashlib

SKETCH = "ESP32_PrusaConnectCam_web"
TARGET = "m5stack-poe-cam-u121"
LAYOUT = "default-4m-ota-1280k-v1"
SLOT_SIZE = 0x140000
BOOTLOADER_SHA256 = "a284abdd0e4339f9ae18030953606a21d9d2f86d41d78ac46d606226f03ba939"
PARTITIONS_SHA256 = "148b959cbff1c38aa8e1d5c0ba9d612c54997b945e56a63f41223eef650653a1"


def source_digest(repo):
    paths = [p for p in (repo / SKETCH).iterdir() if p.suffix in (".h", ".cpp", ".c", ".ino")]
    paths += [repo / "tools" / p for p in ("build.sh", "package_firmware.py", "artifact_meta.py")]
    digest = hashlib.sha256()
    for path in sorted(paths):
        digest.update(path.relative_to(repo).as_posix().encode() + b"\0")
        digest.update(path.read_bytes() + b"\0")
    return digest.hexdigest()


def ota_manifest(version, app):
    name = f"{SKETCH}.ino.bin"
    return {"schema": 1, "version": version, "target": TARGET, "layout": LAYOUT,
            "file": name, "size": len(app), "sha256": hashlib.sha256(app).hexdigest(),
            "url": f"https://github.com/vothmarkus/PrusaConnectCam-M5Stack-PoECam/releases/download/v{version}/{name}"}
