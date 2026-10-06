"""Fail closed if source, firmware, OTA manifest or USB recovery assets drift."""
import hashlib
import json
from pathlib import Path
import re
from artifact_meta import (SKETCH, SLOT_SIZE, BOOTLOADER_SHA256, PARTITIONS_SHA256,
                           ota_manifest, source_digest)


def check(repo):
    output = repo / SKETCH / "build/m5stack.esp32.m5stack_poe_cam"
    manifest = json.loads((output / "manifest.json").read_text())
    version = re.search(r'#define SW_VERSION\s+"([^"]+)"', (repo / SKETCH / "mcu_cfg.h").read_text())[1]
    assert re.fullmatch(r"(0|[1-9]\d{0,4})\.(0|[1-9]\d{0,4})\.(0|[1-9]\d{0,4})", version), "Stable version required"
    assert all(int(p) <= 65535 for p in version.split('.'))
    assert manifest["firmware_version"] == version
    assert manifest["source_sha256"] == source_digest(repo), "Sources changed since build; rebuild first"
    assert manifest["ota"]["bootloader_rollback"] is True and manifest["ota"]["boot_test_override"] is True
    assert manifest["core"] == "m5stack:esp32@3.2.2"
    offsets = {f"{SKETCH}.ino.bin": "0x10000", f"{SKETCH}.ino.bootloader.bin": "0x1000",
               f"{SKETCH}.ino.partitions.bin": "0x8000", f"{SKETCH}.ino.merged.bin": "0x0", "ota-reset.bin": "0xe000"}
    assert set(manifest["images"]) == set(offsets)
    for name, offset in offsets.items():
        data = (output / name).read_bytes()
        info = manifest["images"][name]
        assert info == {"offset": offset, "bytes": len(data), "sha256": hashlib.sha256(data).hexdigest()}, name
    assert (output / "SHA256SUMS").read_text() == "".join(f"{v['sha256']}  {k}\n" for k, v in manifest['images'].items())
    app = (output / f"{SKETCH}.ino.bin").read_bytes()
    assert 288 <= len(app) <= SLOT_SIZE and app[0] == 0xe9 and app[12:14] == b"\0\0"
    assert app[32:36] == bytes.fromhex("3254cdab")
    assert app[144:176].split(b"\0")[0] == b"v5.4.2-25-g858a988d6e"
    assert ("M5PoECAM Prusa Connect " + version).encode() in app
    assert manifest['images'][f'{SKETCH}.ino.bootloader.bin']['sha256'] == BOOTLOADER_SHA256
    assert manifest['images'][f'{SKETCH}.ino.partitions.bin']['sha256'] == PARTITIONS_SHA256
    assert (output / "ota-reset.bin").read_bytes() == b"\xff" * 0x2000
    assert json.loads((output / "ota-manifest.json").read_text()) == ota_manifest(version, app)
    merged = (output / f"{SKETCH}.ino.merged.bin").read_bytes()
    assert len(merged) == 0x400000 and merged[0x9000:0xe000] == b"\xff" * 0x5000
    for suffix, offset in [('bin', 0x10000), ('bootloader.bin', 0x1000), ('partitions.bin', 0x8000)]:
        part = (output / f"{SKETCH}.ino.{suffix}").read_bytes()
        assert merged[offset:offset+len(part)] == part
    html = (repo / "docs/index.html").read_text()
    fw = json.loads(re.search(r"  const FW = (\{.*?\n  \});", html, re.S)[1])
    for entry in fw.values():
        info = manifest['images'][entry['file']]
        assert entry['bytes'] == info['bytes'] and entry['sha256'] == info['sha256']
        assert entry['address'] == int(info['offset'], 16)
    assert all(v == version for v in re.findall(r'data-firmware-version>([^<]+)', html))
    print(f"Release v{version}: source, SDK, rollback gates, OTA slot, hashes, USB/NVS layout and flasher verified")
    return version


if __name__ == "__main__":
    check(Path(__file__).resolve().parent.parent)
