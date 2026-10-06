"""Generate the initial draft's review/test instructions, without publishing."""
import json
from pathlib import Path

repo = Path(__file__).resolve().parent.parent
manifest = json.loads((repo / 'ESP32_PrusaConnectCam_web/build/m5stack.esp32.m5stack_poe_cam/manifest.json').read_text())
version = manifest['firmware_version']
print(f"""## Firmware {version}

M5Stack Unit PoE CAM U121, original M5Stack 3.2.2 SDK and 4 MB partition layout.

- Nightly OTA check between 03:00 and 04:00 Europe/Berlin, staggered per camera.
- Stable, strictly newer releases only; HTTPS certificate verification and SHA-256.
- Inactive-slot installation; startup self-test with bootloader rollback.
- Pairing and settings retained; serial commands: `ota status`, `ota check`, `ota on`, `ota off`.
- USB recovery includes the original bootloader and resets OTA boot selection after writing the app.

**Before publishing:** test this version via USB on one camera and verify snapshots. Publish during daytime, then use `ota check` on a test camera running a lower OTA-capable version to verify the real OTA cycle before the nightly window. Disable automatic updates on other OTA-enabled cameras if necessary. Hardware tests are not performed by GitHub Actions. Publishing this release makes it eligible for automatic installation on older OTA-capable cameras.

Use the [web flasher](https://vothmarkus.github.io/PrusaConnectCam-M5Stack-PoECam/) for the first OTA-capable installation. Keep pairing by choosing Update. The merged image erases pairing. OTA downloads only `ESP32_PrusaConnectCam_web.ino.bin` using `ota-manifest.json`.

See the repository README for test and recovery instructions.
""")
