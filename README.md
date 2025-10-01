# PrusaConnectCam – M5Stack PoE-CAM

[English version](README_en.md)

Dieses Projekt verbindet eine **M5Stack PoE-CAM** direkt mit **Prusa Connect**.  
Die Kamera wird über Ethernet (PoE-fähig) betrieben und meldet sich nach der Kopplung als **externe Kamera** am Drucker in Prusa Connect an.  

---

## Funktionen
- Einfache Einrichtung per **QR-Code-Scan** aus Prusa Connect.  
- Kamera nimmt periodisch Snapshots auf und überträgt diese an Prusa Connect.  
- **Kurzer Tastendruck** an der CAM: QR-Code scannen und verbinden.  
- **Langer Tastendruck**: Zurücksetzen der Kamera (Reset).  
- Ausgabe von **Fehlercodes** über Display (siehe unten).  

---

## Benötigte Hardware
- **M5Stack PoE-CAM** (ESP32 + W5500 Ethernet).  
- Ethernet-Kabel, optional PoE-Switch oder PoE-Injector.  
- Alternativ Stromversorgung über USB-C.  

---

## Installation
### Variante 1: Vorbereitete Firmware (empfohlen)
1. Auf der [Releases-Seite](../../releases) die aktuelle **BIN-Datei mit Bootloader** herunterladen.  
   - ⚠️ Achte darauf, die Version **mit Bootloader** zu nehmen (einmalig beim ersten Flash nötig).  
   - Spätere Updates können auch **ohne Bootloader** installiert werden.  
2. Mit einem Web-Flasher hochladen, z. B.:  
   - [ESPHome Web Flasher](https://web.esphome.io)  
   - oder andere kompatible Web-Flasher für ESP32.  
3. Gerät auswählen → BIN-Datei hochladen → fertig.  

### Variante 2: Manuelles Kompilieren
1. Repo klonen oder als ZIP herunterladen:  
   ```bash
   git clone https://github.com/vothmarkus/PrusaConnectCam-M5Stack-PoECam.git
   ```
2. Arduino IDE oder PlatformIO öffnen.  
3. Projekt `ESP32_PrusaConnectCam_web.ino` öffnen.  
4. Board auswählen: `ESP32 Dev Module` (oder PoE-CAM spezifisch, falls installiert).  
5. Kompilieren und auf die CAM flashen.  

---

## Verwendung
1. **In Prusa Connect anmelden** → Drucker auswählen.  
2. Im Tab **Kameras** → *Neue Kamera hinzufügen*.  
3. Als Typ **Other** auswählen → Prusa zeigt einen **QR-Code** an.  
4. **Kurze Taste** an der M5Stack PoE-CAM drücken.  
5. Kamera scannt den QR-Code und verbindet sich mit Prusa Connect.  
6. Fertig – die Kamera taucht nun im Drucker-Dashboard auf.  

👉 **Reset:** Langer Druck auf die Taste → Gerät zurücksetzen.  

---

## Fehlercodes (Anzeige durch mehrfaches Blinken)
Die Firmware zeigt verschiedene Fehlerzustände an:  

| Code | Bedeutung |
|------|-----------|
| 01   | Fehler beim Kamera-Init |
| 02   | Netzwerk nicht verbunden |
| 03   | Ungültiger QR-Code |
| 04   | Verbindung zu Prusa Connect fehlgeschlagen |
| 05   | Upload Snapshot fehlgeschlagen |

---

## Bekannte Einschränkungen
- Es werden ausschließlich **Snapshots** unterstützt (bedingt durch die Prusa Connect API, kein Live-Stream möglich).  

---

## Referenzen & Anleitungen
- 📖 **Prusa Connect Camera API**: [Prusa Connect Camera API Dokumentation](https://help.prusa3d.com/article/prusa-connect-camera-api_569012)  
- 💡 **Originalprojekt von Prusa Research**: [Prusa-Firmware-ESP32-Cam (GitHub)](https://github.com/prusa3d/Prusa-Firmware-ESP32-Cam)  
- 📝 **Offizielle Anleitung von Prusa zur ESP-Cam**: [ESP Camera for Prusa Connect (Knowledge Base)](https://help.prusa3d.com/guide/esp-camera-for-prusa-connect_390199)  

---

## ToDo
- Eventuell MQTT- oder RTSP-Ausgabe zusätzlich implementieren.  

---

## Lizenz
MIT License
