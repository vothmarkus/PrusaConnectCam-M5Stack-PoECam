# PrusaConnectCam – M5Stack PoE-CAM

This project connects an **M5Stack PoE-CAM** directly to **Prusa Connect**.

The camera is operated via Ethernet (PoE-enabled) and, after pairing, registers as an **external camera** on the printer in Prusa Connect.

---

## Features
- Easy setup via **QR code scan** from Prusa Connect.
- The camera periodically takes snapshots and transmits them to Prusa Connect.
- **Short button press** on the CAM: Scan the QR code and connect.
- **Long button press**: Reset the camera.
- Displays **error codes** on the display (see below).

---

## Required Hardware
- **M5Stack PoE-CAM** (ESP32 + W5500 Ethernet).
- Ethernet cable, optional PoE switch or PoE injector.
- Alternatively, power supply via USB-C.

---

## Installation
### Option 1: Prepared firmware (recommended)
1. Download the latest **BIN file with bootloader** from the [Releases page](../../releases).
   - ⚠️ Make sure to use the version **with bootloader** (required once for the first flash).
   - Later updates can also be installed **without bootloader**.
2. Upload using a web flasher, e.g.:
   - [ESPHome Web Flasher](https://web.esphome.io)
   - or other compatible web flashers for ESP32.
3. Select device → upload BIN file → done.

### Option 2: Manual Compilation
1. Clone the repo or download it as a ZIP file:
   ```bash
   git clone https://github.com/vothmarkus/PrusaConnectCam-M5Stack-PoECam.git
   ```
2. Open Arduino IDE or PlatformIO.
3. Open the project `ESP32_PrusaConnectCam_web.ino`.
4. Select the board: `ESP32 Dev Module` (or PoE-CAM specific, if installed).
5. Compile and flash to the CAM.

---

## Usage
1. **Log in to Prusa Connect** → Select Printer.
2. In the **Cameras** tab → *Add New Camera*.
3. Select **Other** as the type → Prusa displays a **QR code**.
4. **Shortly press** the M5Stack PoE-CAM button.
5. The camera scans the QR code and connects to Prusa Connect.
6. Done – the camera now appears in the printer dashboard.

👉 **Reset:** Long press on the button → Reset device.

---

## Error codes (displayed on the screen)
The firmware displays various error states:

| Code | Meaning |
|------|-----------|
| 01 | Camera initialization error |
| 02 | Network not connected |
| 03 | Invalid QR code |
| 04 | Connection to Prusa Connect failed |
| 05 | Snapshot upload failed |

*(The exact list corresponds to the comments in the source code.)*

---

## Known limitations
- Only **snapshots** are supported (due to the Prusa Connect API, no live stream possible).

---

## References & Guides
- 📖 **Prusa Connect Camera API**: [Prusa Connect Camera API Documentation](https://help.prusa3d.com/article/prusa-connect-camera-api_569012)
- 💡 **Original project from Prusa Research**: [Prusa-Firmware-ESP32-Cam (GitHub)](https://github.com/prusa3d/Prusa-Firmware-ESP32-Cam)
- 📝 **Official Prusa ESP-Cam Guide**: [ESP Camera for Prusa Connect (Knowledge Base)](https://help.prusa3d.com/guide/esp-camera-for-prusa-connect_390199)

---

## ToDo
- Possibly implement MQTT or RTSP output.

---

## License
MIT License
