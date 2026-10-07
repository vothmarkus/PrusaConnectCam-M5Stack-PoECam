"""Generate release notes for the current firmware artifacts."""
import json
from pathlib import Path

repo = Path(__file__).resolve().parent.parent
manifest = json.loads((repo / 'ESP32_PrusaConnectCam_web/build/m5stack.esp32.m5stack_poe_cam/manifest.json').read_text())
version = manifest['firmware_version']
validation = (
    "**1.4.1: version-only OTA test.** The only firmware source change from 1.4.0 is "
    "`SW_VERSION`; camera, upload, SDK, partitions and OTA behavior are unchanged. "
    "The operator confirmed installation of 1.4.0. This regular Latest release is "
    "authorized for the first nightly OTA trial on 8 October 2026, 03:00–04:00 Europe/Berlin. "
    "Leave the test camera on 1.4.0 until then. A successful hardware OTA cycle and "
    "rollback have not yet been confirmed. The boot log and `ota status` report the running version.\n\n"
    "The web flasher now has a bounded, scrolling serial console, connection status, "
    "and a Version / OTA-Status button. Connecting reads status without installing an update."
    if version == "1.4.1" else
    "**Before publishing:** test this version via USB on one camera and verify snapshots. "
    "Publish during daytime, then use `ota check` on a test camera running a lower "
    "OTA-capable version to verify the real OTA cycle before the nightly window. "
    "Hardware tests are not performed by GitHub Actions."
)
print(f"""## Firmware {version}

M5Stack Unit PoE CAM U121, original M5Stack 3.2.2 SDK and 4 MB partition layout.

- Nightly OTA check between 03:00 and 04:00 Europe/Berlin, staggered per camera.
- Stable, strictly newer releases only; HTTPS certificate verification and SHA-256.
- Inactive-slot installation; startup self-test with bootloader rollback.
- Pairing and settings retained; serial commands: `ota status`, `ota check`, `ota on`, `ota off`.
- USB recovery includes the original bootloader and resets OTA boot selection after writing the app.

{validation}

Publishing this regular release makes it eligible for automatic installation on older OTA-capable cameras. Only a matching version and application SHA-256 in `.github/release-publication.json` authorizes workflow publication; other versions remain drafts.

Use the [web flasher](https://vothmarkus.github.io/PrusaConnectCam-M5Stack-PoECam/) for the first OTA-capable installation. Keep pairing by choosing Update. The merged image erases pairing. OTA downloads only `ESP32_PrusaConnectCam_web.ino.bin` using `ota-manifest.json`.

See the repository README for test and recovery instructions.
""")
