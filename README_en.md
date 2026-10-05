# PrusaConnectCam – M5Stack PoE-CAM

[Deutsch](README.md) · Firmware **1.3.1**

The M5Stack **Unit PoE CAM (U121, ESP32 + W5500 + OV2640)** sends JPEG snapshots directly to Prusa Connect. It connects over Ethernet and can be powered by a PoE switch or injector. Defaults: one image every 10 seconds, 1600 × 1200 pixels, JPEG quality setting 20.

## Camera startup failure after updating to 1.3.0

**1.3.1 restores the board package used by the previously working firmware.** The original ELF committed in `1afe580` records **M5Stack 3.2.2**, ESP-IDF `v5.4.2-25-g858a988d6e` and the board options documented below. Version 1.3.0 was accidentally built with **Espressif 3.2.0** and a different SDK. This unnecessary change is the suspected cause of the reported camera startup failure; sensor pins were unchanged. Successful operation after a PC update was confirmed on 5 October 2026.

**8 flashes**, `i2c.master: probe device timeout` and `Camera probe failed ... 0x105 (ESP_ERR_NOT_FOUND)` indicate that sensor detection failed before any HTTPS upload. This particular failure is not caused by an SSL certificate.

Update paired cameras using the **1.3.1 application BIN**, or choose **“Kamera auf 1.3.1 aktualisieren”** in the [web flasher](https://vothmarkus.github.io/PrusaConnectCam-M5Stack-PoECam/). A full erase or re-pairing is not required. Power-cycle after flashing. The log should show `M5PoECAM Prusa Connect 1.3.1`, the SDK above, then `Camera ready`. Check for a fresh Prusa Connect snapshot before updating the remaining cameras.

## Fix for Prusa uploads

During investigation on **3 October 2026**, the previous endpoint `webcam.connect.prusa3d.com/c/snapshot` returned HTTP 301. Its redirect points to the webcam application; the current upload API is **`https://camera-service.prusa3d.com/c/snapshot`**. The old firmware ignored HTTP status codes and could report a failed upload as successful.

The original **ISRG Root X1 has not expired** (valid until June 2035). Version 1.3.0 adds **GTS Root R1 and R4** for the new service while keeping TLS certificate verification enabled. It also adds:

- HTTP status handling, bounded connect/read timeouts and direct binary frame-buffer uploads.
- No automatic forwarding of credentials to redirect targets and no token logging.
- Legacy `?token=…` pairing links and current `#t=v1.…` QR codes.
- Asynchronous NTP startup, working LED error codes and a 30-second QR scan timeout.
- Safer camera/QR memory handling and validation of stored settings.

**A Git commit does not update the cameras automatically.** Flash each camera using a programmer; this firmware has no OTA update feature. Update one camera and verify actual snapshots before updating the rest.

## Install prebuilt firmware

An **external ESP32 Downloader with the appropriate PoE-CAM adapter** is required. A USB cable alone is not a programmer. See the [M5Stack hardware instructions](https://docs.m5stack.com/en/unit/Unit_PoE_CAM) for connections and download mode. The hardware has 16 MB flash; these images retain the project's existing **4 MB partition layout**.

| Purpose | Download | Flash offset | Existing pairing |
| --- | --- | --- | --- |
| Update an existing installation of this project | [Application BIN](https://raw.githubusercontent.com/vothmarkus/PrusaConnectCam-M5Stack-PoECam/main/ESP32_PrusaConnectCam_web/build/m5stack.esp32.m5stack_poe_cam/ESP32_PrusaConnectCam_web.ino.bin) | `0x10000` | Preserved if the entire flash is not erased |
| Initial installation / complete reinstall | [Merged BIN](https://raw.githubusercontent.com/vothmarkus/PrusaConnectCam-M5Stack-PoECam/main/ESP32_PrusaConnectCam_web/build/m5stack.esp32.m5stack_poe_cam/ESP32_PrusaConnectCam_web.ino.merged.bin) | `0x0` | Erased; pair again afterward |

**Use the application BIN for paired cameras and leave “Erase all flash” disabled.** The merged image includes an empty settings area and overwrites stored tokens/fingerprints. Application-only updates assume this repository's existing partition layout, not arbitrary third-party firmware.

Using Python and esptool 4.8.1 from the download directory:

```bash
python -m pip install esptool==4.8.1
# Replace /dev/ttyUSB0 with your port, e.g. COM5 on Windows.
python -m esptool --chip esp32 --port /dev/ttyUSB0 --baud 460800 write_flash 0x10000 ESP32_PrusaConnectCam_web.ino.bin
```

For **initial installation** instead:

```bash
python -m esptool --chip esp32 --port /dev/ttyUSB0 --baud 460800 write_flash 0x0 ESP32_PrusaConnectCam_web.ino.merged.bin
```

Restart after flashing. If the programmer connection is unreliable, retry with `--baud 115200`. A web flasher is suitable only if it supports the required offset and erase behavior. [Checksums and build metadata](ESP32_PrusaConnectCam_web/build/m5stack.esp32.m5stack_poe_cam/) accompany the binaries. The legacy directory name is retained for existing download links.

### Android USB flasher

Open the [web flasher](https://vothmarkus.github.io/PrusaConnectCam-M5Stack-PoECam/) directly in Chrome over HTTPS. The presence of `navigator.serial` does **not** guarantee USB support on Android: it may expose only Bluetooth. Native wired serial support also depends on Android version and device configuration ([Chromium announcement](https://groups.google.com/a/chromium.org/g/blink-dev/c/HBJ-uYFvkpM/m/MrLnwZlsAAAJ)).

Android now defaults to **“Android USB (CH9102)”**, using WebUSB and a pinned Google CDC-ACM polyfill for the M5Stack downloader with USB ID **`1a86:55d4`**. Firmware and esptool-js are unchanged. This path does not support older **CP2104** downloaders; use those on a PC.

Connect the phone through a USB-C **OTG/host adapter with a USB-A socket** and an A-to-C **data cable** to the downloader. Start at **115200 baud**. A connector adapter alone does not guarantee USB host support; direct C-to-C requires compatible USB-C wiring in the downloader. Press **“Downloader erkennen”**, choose the CH9102 USB device and grant access. This detects and restarts the ESP32 without writing firmware or erasing pairing. Use the application update afterward if needed.

A Bluetooth-only list means native serial mode or an older page is still active. If the USB list is also empty, check OTG/host mode, data cable and power, and close other USB applications. The polyfill and license are in `docs/vendor/`. Software checks pass; a physical Android flash test is still pending.

## Pairing and controls

1. Open the printer in Prusa Connect, add an external camera and display its pairing QR code.
2. **Briefly press and release** the camera's side button. The blue LED flashes quickly during a scan of up to 30 seconds.
3. Keep the complete QR code well lit and visible. A valid token is saved and the camera returns to JPEG capture.
4. After Ethernet and clock synchronization are ready, verify a fresh snapshot in Prusa Connect.

Another short press cancels scanning. Cancellation, an invalid code or a timeout preserves previous pairing. QR codes containing a raw 20-character alphanumeric camera token are also supported. The public `v1` link obfuscation is not encryption and may change in Prusa's frontend.

**Hold the button for 5 seconds:** toggle horizontal mirroring, save and reboot. This is **not a factory reset** and preserves pairing. Each camera needs its own pairing token. Do not clone a paired camera's complete flash contents onto another camera.

## LED signals and troubleshooting

Serial monitor: **115200 baud**. Status is shown by the blue LED; there is no display.

| Continuous blinking | State |
| --- | --- |
| 1 second on / 1 second off | Ethernet has no IP address |
| 0.25 seconds on / off | Waiting for NTP; HTTPS requires a valid clock |
| 2 seconds on / off | No valid camera token stored |
| Fast while scanning | QR recognition active |
| Very fast while holding the button | Mirroring and reboot after 5 seconds |

| Discrete blink sequence after an attempt | Meaning |
| --- | --- |
| 1 | Successful upload (HTTP 2xx); also confirms saving a QR token |
| 2 | DNS, TCP, TLS or transfer failure; inspect serial output |
| 3 | HTTP 401: unauthorized |
| 4 | HTTP 403: access denied; check token, fingerprint and pairing |
| 5 | Invalid token passed to the upload function |
| 6 | Missing or invalid fingerprint |
| 7 | Other HTTP error, including 301, 404, 429 or 5xx; status is logged |
| 8 | Camera initialization or capture failed |

The network must allow DHCP/DNS, **NTP over UDP 123** to `pool.ntp.org` or `time.nist.gov`, and **HTTPS over TCP 443** to `camera-service.prusa3d.com`. Captive portals and authenticated proxies are unsupported. A school firewall's TLS interception certificate is not automatically trusted; ask the network administrator to allow the connection appropriately.

For 401/403, check that the camera still exists in Prusa Connect and belongs to the intended printer; obtain a new pairing QR code if needed. For `Still waiting for NTP`, check access to the time servers. For HTTP 301/404, check the configured API host. Address TLS failures by updating the relevant CA certificates, not by disabling verification.

## Build and test

Reference build: **Arduino CLI 1.3.1**, **M5Stack 3.2.2** (Arduino core 3.2.1, ESP-IDF `v5.4.2-25-g858a988d6e`), board `M5PoECAM` (`m5stack:esp32:m5stack_poe_cam`). Arduino libraries are included in the core; `quirc` is bundled. `ArduinoUniqueID` is no longer required. No PlatformIO configuration is provided.

```bash
git clone https://github.com/vothmarkus/PrusaConnectCam-M5Stack-PoECam.git
cd PrusaConnectCam-M5Stack-PoECam
arduino-cli core update-index --additional-urls https://static-cdn.m5stack.com/resource/arduino/package_m5stack_index.json
arduino-cli core install m5stack:esp32@3.2.2 --additional-urls https://static-cdn.m5stack.com/resource/arduino/package_m5stack_index.json
./tools/build.sh
```

The script requires Bash/Python 3 and selects PSRAM **enabled**, **default** partitions, flash **4 MB / QIO / 80 MHz**, CPU **240 MHz**, loop/event core **1**, debug **none**, and full-flash erase **disabled**. In Arduino IDE, select the same options and open `ESP32_PrusaConnectCam_web/ESP32_PrusaConnectCam_web.ino`. The script exports binaries, `manifest.json` and `SHA256SUMS` to the existing build directory; intermediate ELF/MAP files stay under `.build/`. Export also updates the web flasher version, image sizes and checksums.

M5Stack package 3.2.2 internally reports Arduino core 3.2.1; this is expected. Its unmodified headers produce pin macro redefinition warnings. Do not switch board packages just to remove these vendor warnings.

Host tests require Linux, GCC/G++ and Bash:

```bash
./tests/run.sh
```

Tests cover HTTP statuses, both pairing URL formats, malformed input, EEPROM bounds, actual QR recognition including mirrored images, and allocation failures under AddressSanitizer/UndefinedBehaviorSanitizer. If LeakSanitizer cannot run in your environment, use `ASAN_OPTIONS=detect_leaks=0 ./tests/run.sh`; QR tests also count outstanding allocations.

Web flasher tests (Node.js 22+): `node --test tests/web_flasher_test.mjs`. These cover Android exposing Bluetooth Serial and WebUSB at once, CDC control requests, in-place baud changes, selection before downloads, hash/chip rejection and the pairing-preserving update offset.

**Validation limit:** compilation and software tests do not replace testing a physical camera with valid Prusa credentials. Check cold boot, fresh snapshots, network loss/recovery, QR pairing and retained settings after reboot.

## Scope and sources

This firmware provides Ethernet snapshots. A local web interface, RTSP, MQTT and OTA are not implemented; the historical `_web` sketch name does not indicate a web interface.

- [Prusa Camera API](https://connect.prusa3d.com/docs/cameras/) and [token/fingerprint communication](https://connect.prusa3d.com/docs/cameras/camera_communication/)
- [Public Prusa webcam app](https://camera-service-webcam.prusa3d.com/) – API configuration and QR link format observed on 2026-10-03
- [Google Trust Services roots](https://pki.goog/repository/) and [ISRG certificates](https://letsencrypt.org/certificates/)
- [M5Stack Unit PoE CAM](https://docs.m5stack.com/en/unit/Unit_PoE_CAM) and [Arduino guide](https://docs.m5stack.com/en/arduino/m5poe_cam/program)

Project: Markus Voth. Project license: MIT. Bundled `quirc` and OpenMV files retain their own license and copyright notices in their source headers.
