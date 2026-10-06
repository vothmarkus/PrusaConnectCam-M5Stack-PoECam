# PrusaConnectCam – M5Stack PoE-CAM

[Deutsch](README.md) · Firmware **1.4.0** · [Web flasher](https://vothmarkus.github.io/PrusaConnectCam-M5Stack-PoECam/)

The **M5Stack Unit PoE CAM U121** (ESP32, W5500, OV2640) uploads JPEG snapshots to Prusa Connect over Ethernet. A PoE switch/injector supplies power. Defaults: one snapshot every 10 seconds, 1600 × 1200, JPEG quality value 20.

**New in 1.4.0:** nightly updates from published GitHub releases, verified HTTPS downloads, startup self-test and bootloader rollback. The working **M5Stack 3.2.2** board package, camera pins and partition layout are retained. The original bootloader is byte-identical.

**Validation status:** firmware builds and software tests pass. Previous firmware 1.3.1 was successfully flashed on a PC and Android, including the higher baud rate according to user feedback. A real OTA cycle and rollback for 1.4.0 still require testing on one camera before deploying to the fleet.

## Install OTA support once

1. Connect an **external ESP32 Downloader with the matching PoE-CAM adapter**. A USB cable alone is not a programmer. See [M5Stack](https://docs.m5stack.com/en/unit/Unit_PoE_CAM).
2. Open the [web flasher](https://vothmarkus.github.io/PrusaConnectCam-M5Stack-PoECam/) directly in desktop Chrome/Edge or Android Chrome.
3. Choose **“Kamera auf 1.4.0 aktualisieren”**. Existing pairing is preserved.
4. Start the camera on the PoE network. The serial log should show `M5PoECAM Prusa Connect 1.4.0`, `Camera ready`, and `OTA self-test passed` after at least 30 seconds.
5. Verify a fresh snapshot in Prusa Connect.

Every camera needs this initial USB installation once. Later application updates can use Ethernet. Versions up to 1.3.1 cannot update themselves. **Full installation erases pairing** and is unnecessary for a normal update.

### Android USB

Select **“Android USB (CH9102)”** for the M5Stack downloader with USB ID **`1a86:55d4`**. Older CP2104 downloaders require desktop native Serial.

- Open the HTTPS page directly in Chrome, outside embedded app browsers.
- Use a USB host/OTG adapter and data cable.
- Choose **“Downloader erkennen”** and allow `USB-Enhanced-SERIAL CH9102`. This connection test does not flash firmware.
- Android defaults to 115200 baud; 460800 may be selected with a stable connection.
- A Bluetooth-only picker indicates native Serial mode or an outdated page. `navigator.serial` alone does not imply wired Android USB support ([Chromium](https://groups.google.com/a/chromium.org/g/blink-dev/c/HBJ-uYFvkpM/m/MrLnwZlsAAAJ)).

## Automatic updates

Each camera checks **once per night between 03:00 and 04:00 Europe/Berlin**, including daylight saving time. Its MAC address determines a fixed second within that hour. The boot log and `ota status` show the scheduled time and last attempted day.

Ethernet and a valid NTP clock are required. If unavailable at the scheduled time, the camera may catch up within the same hour. After 04:00 it waits until the next night. The attempt date is saved before checking, so rebooting does not repeatedly trigger a nightly check. Network/TLS/download failures leave the existing firmware running; automatic retry occurs the next night.

1. Fetch `https://github.com/vothmarkus/PrusaConnectCam-M5Stack-PoECam/releases/latest/download/ota-manifest.json`. GitHub's latest release excludes drafts and prereleases.
2. Validate schema, hardware target, partition layout, and a **strictly newer** stable `major.minor.patch` version. Never downgrade.
3. Download the release's application BIN over verified HTTPS, using the SDK's full CA bundle. Check every redirect's HTTPS host. Camera tokens/fingerprints are never sent to GitHub.
4. Write the inactive application slot, verify **size, SHA-256 and ESP32 image**, then select it for the next boot. OTA never writes the merged image, bootloader, partition table or pairing area.
5. Reboot, run the startup self-test and confirm the new application. Snapshots pause during download/reboot.

A commit to `main` does not trigger installation on cameras. **Publishing a release authorizes rollout.** If the latest release has no `ota-manifest.json`, a 404 is expected and the camera continues normally. The old `V1.2.0` release predates OTA support.

### Self-test and rollback

Confirmation requires PSRAM, a successful JPEG capture, successful Ethernet driver initialization and at least 30 seconds of uptime. It does not depend on NTP, Prusa Connect or internet availability. It also cannot detect every later functional regression; verify actual Prusa snapshots separately.

A failed test rolls back to the previous application. An unconfirmed reboot also triggers bootloader rollback. An additional timer restarts an unconfirmed trial after 120 seconds if startup hangs. The previous firmware remembers the rejected version and will skip it; fixes must use a higher version.

The first USB installation records its application slot as a valid fallback for subsequent OTA updates. USB repair/interrupted initial installation does not provide the same protection as inactive-slot OTA. Keep the programmer available for recovery.

### Serial commands

Use **115200 baud** and terminate each command with a **newline**:

| Command | Effect |
| --- | --- |
| `ota status` | Firmware, enabled state, schedule, last check day, rejected version |
| `ota check` | Queue an immediate check; waits for self-test, Ethernet and NTP |
| `ota off` | Persistently disable automatic nightly updates |
| `ota on` | Enable nightly updates again |

Manual `ota check` works even when automatic updates are disabled. Rebooting preserves `ota off`. Send these commands to running firmware, not the ROM download mode. The web flasher does not include a serial monitor.

## Preparing and testing a release

1. Increase `SW_VERSION` in `mcu_cfg.h` and run `./tools/build.sh`. Commit sources, all images and generated metadata together.
2. **“Validate firmware and prepare release”** checks the sources/artifacts, tests, hashes and USB flasher. It creates or updates a **draft `v<version>` release** containing the files. Published firmware is never overwritten.
3. Test the new firmware via USB on one camera first. For the first actual OTA transfer, keep another test installation on a lower OTA-capable version, such as 1.4.0.
4. Publish the tested draft as a normal release during daytime; mark it **Latest** if necessary. Run `ota check` on the test camera and verify the transfer, reboot and snapshots before the nightly window. Use `ota off` beforehand on other OTA-enabled cameras when needed.
5. Hardware acceptance also includes switching back into the other slot, interrupting a download, and deliberately failing a startup self-test on the test camera. Never publish a deliberately broken test image as the fleet's regular release.

A startup failure test may use Espressif's `otatool.py` to write a test image to the inactive slot and select it for boot, with the programmer available and the working fallback slot recorded. Do not intentionally introduce failures on production cameras.

## USB recovery and assets

The hardware has 16 MB flash; this project retains its compatible **4 MB layout**. Both OTA slots are **1280 KiB**; firmware 1.4.0 is about 983 KiB.

[Images, checksums and metadata](ESP32_PrusaConnectCam_web/build/m5stack.esp32.m5stack_poe_cam/) retain their existing download directory.

| File | Offset / purpose |
| --- | --- |
| `ESP32_PrusaConnectCam_web.ino.bin` | OTA application; USB offset `0x10000` |
| `ESP32_PrusaConnectCam_web.ino.bootloader.bin` | Original M5Stack bootloader, `0x1000` |
| `ota-reset.bin` | 8 KiB of `0xFF` at `0xe000`, **only after successful application/bootloader writes** |
| `ESP32_PrusaConnectCam_web.ino.partitions.bin` | Existing table at `0x8000`; unchanged during normal updates |
| `ESP32_PrusaConnectCam_web.ino.merged.bin` | Full installation at `0x0`; erases pairing |
| `ota-manifest.json` | Version, target, layout, size, SHA-256 and application release URL |

The web flasher verifies every download before writing. Update mode writes the application and original bootloader with `eraseAll: false`, then resets only the two OTA metadata sectors. **NVS at `0x9000–0xdfff` is preserved.**

Writing only an app at `0x10000` after OTA may leave `app1` selected. The final `ota-reset.bin` write activates the USB installation. Never reset boot selection after a failed application write. These instructions apply only to this project's existing layout.

## Pairing and controls

1. Add an external camera to the printer in Prusa Connect and display its pairing QR code.
2. Briefly press/release the camera's side button. The LED flashes quickly during a scan lasting up to 30 seconds.
3. Present the complete, well-lit QR code. Confirm a fresh snapshot after pairing.

A second short press cancels scanning while preserving previous pairing. Supported QR formats: raw 20-character alphanumeric token, legacy `?token=…` URLs and new `#t=v1.…` URLs.

**Hold for 5 seconds:** toggle horizontal mirroring, save and reboot. This is not a factory reset. Each camera needs a separate token; do not clone a paired camera's complete flash onto other devices.

## LED and troubleshooting

| Repeated pattern | State |
| --- | --- |
| 1 second on/off | Ethernet has no IP |
| 0.25 seconds on/off | Waiting for NTP |
| 2 seconds on/off | No pairing token |
| Fast during scanning | QR scan |
| Very fast while held | Mirror/reboot after 5 seconds |

| Single burst | Result |
| --- | --- |
| 1 blink | HTTP 2xx success / pairing confirmation |
| 2 blinks | DNS, TCP, TLS or transfer error |
| 3 / 4 blinks | HTTP 401 / 403; check pairing |
| 5 / 6 blinks | Invalid token / fingerprint |
| 7 blinks | Other HTTP error: e.g. 301, 404, 429, 5xx |
| 8 blinks | Camera initialization/capture failure |

Allow DHCP/DNS, NTP **UDP 123** to `pool.ntp.org` or `time.nist.gov`, and HTTPS **TCP 443** to `camera-service.prusa3d.com`. OTA additionally needs `github.com`, `release-assets.githubusercontent.com` and possibly `objects.githubusercontent.com`. Captive portals/authenticated proxies are unsupported. TLS inspection requires an appropriate network exemption or deliberately configured trust chain, not disabled certificate verification.

**October 2026 upload repair:** the old `webcam.connect.prusa3d.com/c/snapshot` endpoint returned 301. Firmware now uses `https://camera-service.prusa3d.com/c/snapshot`, checks HTTP status and timeouts, and refuses redirects carrying camera credentials. GTS Root R1/R4 were added alongside ISRG Root X1; ISRG Root X1 had not expired.

**1.3.0 sensor regression:** an unintended change from M5Stack 3.2.2 to Espressif 3.2.0 selected a different SDK. Version 1.3.1 restored the original package and operation was confirmed. Eight blinks and `Camera probe failed ... 0x105` indicate a sensor problem before HTTPS. Version 1.4.0 retains the restored ESP-IDF `v5.4.2-25-g858a988d6e`.

## Build and software tests

Reference: **Arduino CLI 1.3.1**, **M5Stack 3.2.2**, Arduino core 3.2.1, ESP-IDF `v5.4.2-25-g858a988d6e`. No extra Arduino libraries are required. JSON, HTTPS/CA bundle and OTA come from the SDK; `quirc` is bundled.

```bash
git clone https://github.com/vothmarkus/PrusaConnectCam-M5Stack-PoECam.git
cd PrusaConnectCam-M5Stack-PoECam
arduino-cli core update-index --additional-urls https://static-cdn.m5stack.com/resource/arduino/package_m5stack_index.json
arduino-cli core install m5stack:esp32@3.2.2 --additional-urls https://static-cdn.m5stack.com/resource/arduino/package_m5stack_index.json
./tools/build.sh
```

Options: PSRAM **enabled**, **default** partitions, flash **4 MB / QIO / 80 MHz**, CPU **240 MHz**, loop/event core **1**, debug **none**, full erase **disabled**. Packaging checks the original SDK, bootloader, partition hash, image size and actually linked Arduino rollback hook, then updates all images/metadata and the flasher. ELF/MAP intermediates remain under `.build/`.

The vendor package reports Arduino core 3.2.1 internally and emits duplicate pin macro warnings. Do not change the board package to suppress them.

Linux test dependencies: GCC/G++, Bash, OpenSSL development headers (`libssl-dev`), Python 3 and Node.js ≥22.

```bash
./tests/run.sh
node --test tests/web_flasher_test.mjs
python3 tools/check_release.py
```

Tests cover HTTP/QR/EEPROM, real QR decoding and allocation failures, OTA version/schedule/redirect policies, the production streaming logic including SHA-256/truncation/write failures, Android USB and recovery ordering. AddressSanitizer/UndefinedBehaviorSanitizer are enabled. If LeakSanitizer is unavailable: `ASAN_OPTIONS=detect_leaks=0 ./tests/run.sh`.

These checks do not emulate complete ESP32 hardware. Cold boot, actual OTA, both slots, rollback, Prusa uploads and retained pairing additionally require device testing.

## Sources and license

- [Prusa Camera API](https://connect.prusa3d.com/docs/cameras/)
- [ESP-IDF 5.4.2 OTA/rollback](https://docs.espressif.com/projects/esp-idf/en/v5.4.2/esp32/api-reference/system/ota.html)
- [GitHub Releases](https://docs.github.com/en/rest/releases/releases)
- [M5Stack Unit PoE CAM](https://docs.m5stack.com/en/unit/Unit_PoE_CAM)
- [Google Trust Services](https://pki.goog/repository/) / [ISRG certificates](https://letsencrypt.org/certificates/)

Project: Markus Voth, MIT license. Bundled `quirc`, OpenMV and the unmodified Google Web Serial polyfill retain their respective notices. Firmware provides Ethernet snapshots and OTA; local web UI, RTSP and MQTT are not implemented.
