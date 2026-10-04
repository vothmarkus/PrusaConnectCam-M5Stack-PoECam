# PrusaConnectCam – M5Stack PoE-CAM

[English](README_en.md) · Firmware **1.3.1**

Die M5Stack **Unit PoE CAM (U121, ESP32 + W5500 + OV2640)** sendet JPEG-Schnappschüsse direkt an Prusa Connect. Ethernet übernimmt die Netzwerkverbindung, ein PoE-Switch oder PoE-Injector die Stromversorgung. Standard: ein Bild alle 10 Sekunden, 1600 × 1200 Pixel, JPEG-Qualitätswert 20.

## Kamera-Startfehler nach Update auf 1.3.0

**1.3.1 stellt das Board-Paket der zuvor funktionierenden Firmware wieder her.** Die im ursprünglichen Commit `1afe580` gespeicherte ELF-Datei belegt **M5Stack 3.2.2**, ESP-IDF `v5.4.2-25-g858a988d6e` und die unten dokumentierten Board-Optionen. Beim Build von 1.3.0 wurde versehentlich **Espressif 3.2.0** mit einem anderen SDK verwendet. Dieser unnötige Wechsel ist die vermutete Ursache des gemeldeten Kamera-Startfehlers; die Sensor-Pins waren unverändert. Die Bestätigung am Gerät steht noch aus.

**8-maliges Blinken**, `i2c.master: probe device timeout` und `Camera probe failed ... 0x105 (ESP_ERR_NOT_FOUND)` bedeuten: Der Kamerasensor wird beim Start nicht erkannt. Dieser Fehler tritt vor dem HTTPS-Upload auf und wird nicht durch ein SSL-Zertifikat verursacht.

Bereits gekoppelte Geräte mit der **Update-BIN 1.3.1** oder im [Web-Flasher](https://vothmarkus.github.io/PrusaConnectCam-M5Stack-PoECam/) über **„Kamera auf 1.3.1 aktualisieren“** aktualisieren. Kein vollständiges Löschen und keine erneute Kopplung sind dafür erforderlich. Nach dem Flashen einmal stromlos machen und neu starten. Im Protokoll müssen `M5PoECAM Prusa Connect 1.3.1`, das oben genannte SDK und anschließend `Camera ready` erscheinen. Danach einen neuen Schnappschuss in Prusa Connect prüfen, bevor die übrigen Kameras aktualisiert werden.

## Reparatur der Prusa-Uploads

Bei der Untersuchung am **3. Oktober 2026** antwortete der bisherige Upload-Endpunkt `webcam.connect.prusa3d.com/c/snapshot` mit einer HTTP-301-Weiterleitung. Deren Ziel ist die Webcam-Webanwendung; die aktuelle Upload-API liegt unter **`https://camera-service.prusa3d.com/c/snapshot`**. Der alte Code wertete den HTTP-Status nicht aus und konnte einen fehlgeschlagenen Upload als Erfolg anzeigen.

Das bisher eingebettete **ISRG Root X1 ist nicht abgelaufen** (gültig bis Juni 2035). Für den neuen Dienst ergänzt 1.3.0 **GTS Root R1 und R4**. Die TLS-Zertifikatsprüfung bleibt aktiv. Weitere Änderungen:

- HTTP-Statusauswertung, begrenzte Verbindungs-/Lesezeiten und direkte Übertragung des JPEG-Puffers.
- Keine automatische Weitergabe von Token und Fingerprint an Weiterleitungsziele; keine Token im seriellen Protokoll.
- Unterstützung bisheriger `?token=…`-Links und neuer `#t=v1.…`-QR-Codes.
- Asynchroner NTP-Start, funktionierende LED-Fehlercodes und ein QR-Scan mit 30-Sekunden-Zeitlimit.
- Abgesicherte Kamera-/QR-Speicherverwaltung sowie Prüfung gespeicherter Einstellungen.

**Ein Git-Update aktualisiert die Kameras nicht automatisch.** Jede Kamera muss per Programmer geflasht werden; diese Firmware enthält kein OTA-Update. Zuerst eine Kamera aktualisieren und ihren tatsächlichen Bild-Upload prüfen, anschließend die übrigen Geräte.

## Fertige Firmware installieren

Die PoE-CAM benötigt einen **externen ESP32 Downloader mit passendem PoE-CAM-Adapter**. Ein USB-Kabel allein ist kein Programmer. Anschluss und Download-Modus stehen in der [M5Stack-Anleitung](https://docs.m5stack.com/en/unit/Unit_PoE_CAM). Die Hardware hat 16 MB Flash; diese Firmware behält das bisherige **4-MB-Partitionslayout** bei.

| Zweck | Datei | Flash-Adresse | Gespeicherte Kopplung |
| --- | --- | --- | --- |
| Vorhandene Installation dieses Projekts aktualisieren | [Update-BIN herunterladen](https://raw.githubusercontent.com/vothmarkus/PrusaConnectCam-M5Stack-PoECam/main/ESP32_PrusaConnectCam_web/build/m5stack.esp32.m5stack_poe_cam/ESP32_PrusaConnectCam_web.ino.bin) | `0x10000` | Bleibt erhalten, wenn kein vollständiges Löschen erfolgt |
| Erstinstallation / vollständige Neuinstallation | [Merged-BIN herunterladen](https://raw.githubusercontent.com/vothmarkus/PrusaConnectCam-M5Stack-PoECam/main/ESP32_PrusaConnectCam_web/build/m5stack.esp32.m5stack_poe_cam/ESP32_PrusaConnectCam_web.ino.merged.bin) | `0x0` | Wird gelöscht; anschließend neu koppeln |

**Für bereits gekoppelte Kameras die Update-BIN verwenden und „Erase all flash“ deaktiviert lassen.** Das Merged-Image enthält auch den leeren Einstellungsbereich und überschreibt vorhandene Token/Fingerprints. Die Update-Anleitung gilt für das bisherige Partitionslayout dieses Repositories, nicht für beliebige Fremdfirmware.

Beispiel mit Python und esptool 4.8.1, aus dem Download-Ordner:

```bash
python -m pip install esptool==4.8.1
# /dev/ttyUSB0 durch den eigenen Port ersetzen, unter Windows z. B. COM5.
python -m esptool --chip esp32 --port /dev/ttyUSB0 --baud 460800 write_flash 0x10000 ESP32_PrusaConnectCam_web.ino.bin
```

Für die **Erstinstallation** stattdessen:

```bash
python -m esptool --chip esp32 --port /dev/ttyUSB0 --baud 460800 write_flash 0x0 ESP32_PrusaConnectCam_web.ino.merged.bin
```

Danach die Kamera neu starten. Bei Verbindungsproblemen zum Programmer mit `--baud 115200` wiederholen. [Prüfsummen und Build-Metadaten](ESP32_PrusaConnectCam_web/build/m5stack.esp32.m5stack_poe_cam/) liegen bei den BIN-Dateien. Der alte Verzeichnisname bleibt für bestehende Download-Links erhalten.

### Web-Flasher

Unter [`docs/index.html`](docs/index.html) liegt ein browserbasierter Flasher auf Basis von **Espressif esptool-js 0.7.0**. Er trennt die beiden Anwendungsfälle bewusst:

- **Bestehende Kamera aktualisieren:** schreibt ausschließlich die App-BIN nach `0x10000` mit `eraseAll: false`. Der Einstellungsbereich wird nicht vollständig gelöscht.
- **Vollständige Neuinstallation:** löscht den Flash und schreibt das Merged-Image nach `0x0`. Token/Fingerprint und Kopplung gehen verloren.
- Vor dem Flashen werden Dateigröße und **SHA-256** gegen die zu Firmware 1.3.1 gehörenden Build-Metadaten geprüft.
- Standardbaudrate ist 460800; bei Verbindungsproblemen kann direkt auf 115200 umgestellt werden.

Für den Web-Flasher wird weiterhin der externe ESP32-Downloader/PoE-CAM-Adapter benötigt. Die Seite muss über **HTTPS** ausgeliefert werden, beispielsweise über GitHub Pages; lokal per `file://` steht Web Serial nicht zuverlässig zur Verfügung.

**Android:** Chrome unterstützt die Web Serial API seit Version 148 auch auf Android. Der Flasher verwendet dort bewusst denselben bereits funktionierenden Web-Serial-Pfad wie am Desktop; es gibt keinen separaten experimentellen Flash-Algorithmus. Benötigt werden ein Android-Gerät mit USB-Host/OTG, ein Datenkabel/Adapter und der M5Stack-Downloader. Die Seite zeigt direkt an, ob `navigator.serial` im verwendeten Browser verfügbar ist.

## Koppeln und bedienen

1. In Prusa Connect den Drucker öffnen und eine externe Kamera hinzufügen. Den dort angebotenen Kopplungs-QR-Code anzeigen.
2. Die Seitentaste der PoE-CAM **kurz drücken und loslassen**. Die blaue LED blinkt schnell, der Scan läuft maximal 30 Sekunden.
3. Den QR-Code gut beleuchtet und vollständig ins Bild halten. Bei Erfolg speichert die Kamera ihren Token und wechselt zurück zu JPEG-Aufnahmen.
4. Nach Netzwerkverbindung und Zeitsynchronisation den neuen Schnappschuss in Prusa Connect prüfen.

Ein weiterer kurzer Tastendruck bricht den Scan ab. Bei Abbruch, ungültigem QR-Code oder Zeitüberschreitung bleibt eine vorhandene Kopplung erhalten. Unterstützt werden auch QR-Codes mit dem reinen 20-stelligen alphanumerischen Kamera-Token. Die öffentliche `v1`-Verschleierung neuer Prusa-Links ist kein Verschlüsselungsverfahren und kann sich serverseitig ändern.

**Taste 5 Sekunden halten:** horizontale Spiegelung umschalten, speichern und neu starten. Das ist **kein Werksreset**; die Kopplung bleibt erhalten. Jede Kamera benötigt ihren eigenen Kopplungs-Token. Einen vollständigen Flash-Abzug einer gekoppelten Kamera nicht auf andere Geräte kopieren.

## LED und Fehlersuche

Serieller Monitor: **115200 Baud**. Die blaue LED signalisiert den Zustand; diese Kamera hat kein Display.

| Wiederholtes Blinken | Zustand |
| --- | --- |
| 1 Sekunde an / 1 Sekunde aus | Ethernet hat noch keine IP-Adresse |
| 0,25 Sekunden an / aus | NTP-Zeit fehlt; HTTPS wartet auf eine gültige Uhrzeit |
| 2 Sekunden an / aus | Kein gültiger Kamera-Token gespeichert |
| Schnell während des Scans | QR-Erkennung aktiv |
| Sehr schnell bei gehaltener Taste | Nach 5 Sekunden Spiegelung und Neustart |

| Einzelne Blinkfolge nach einem Versuch | Bedeutung |
| --- | --- |
| 1 × | Upload erfolgreich (HTTP 2xx); auch Bestätigung eines gespeicherten QR-Tokens |
| 2 × | DNS-, TCP-, TLS- oder Übertragungsfehler; serielles Protokoll prüfen |
| 3 × | HTTP 401: nicht autorisiert |
| 4 × | HTTP 403: Kamera-Zugriff verweigert; Token/Fingerprint/Kopplung prüfen |
| 5 × | Ungültiger Token beim Upload-Aufruf |
| 6 × | Fehlender oder ungültiger Fingerprint |
| 7 × | Anderer HTTP-Fehler, z. B. 301, 404, 429 oder 5xx; Status im Protokoll |
| 8 × | Kamera-Initialisierung oder Aufnahme fehlgeschlagen |

Im Schulnetz müssen DHCP/DNS, **NTP über UDP 123** zu `pool.ntp.org` oder `time.nist.gov` und **HTTPS über TCP 443** zu `camera-service.prusa3d.com` funktionieren. Captive Portals und authentifizierte Proxys unterstützt die Firmware nicht. Bei einer TLS-Prüfung durch die Schul-Firewall ist deren Zertifikat nicht automatisch vertrauenswürdig; die Netzwerkadministration sollte die Verbindung entsprechend freigeben.

Bei 401/403 zuerst prüfen, ob die Kamera in Prusa Connect noch existiert und zum richtigen Drucker gehört. Falls nötig einen neuen Kopplungs-QR-Code verwenden. Bei `Still waiting for NTP` die Zeitserver-Erreichbarkeit prüfen. Bei HTTP 301/404 den konfigurierten API-Host prüfen. TLS-Probleme durch Aktualisieren der passenden CA-Zertifikate lösen, nicht durch Abschalten der Prüfung.

## Selbst kompilieren und testen

Referenz-Build: **Arduino CLI 1.3.1**, **M5Stack 3.2.2** (Arduino-Core 3.2.1, ESP-IDF `v5.4.2-25-g858a988d6e`), Board `M5PoECAM` (`m5stack:esp32:m5stack_poe_cam`). Es werden nur Bibliotheken aus diesem Core sowie das mitgelieferte `quirc` benötigt; `ArduinoUniqueID` ist nicht mehr erforderlich. PlatformIO-Konfigurationen sind nicht enthalten.

```bash
git clone https://github.com/vothmarkus/PrusaConnectCam-M5Stack-PoECam.git
cd PrusaConnectCam-M5Stack-PoECam
arduino-cli core update-index --additional-urls https://static-cdn.m5stack.com/resource/arduino/package_m5stack_index.json
arduino-cli core install m5stack:esp32@3.2.2 --additional-urls https://static-cdn.m5stack.com/resource/arduino/package_m5stack_index.json
./tools/build.sh
```

Das Build-Skript benötigt Bash/Python 3 und setzt PSRAM **enabled**, Partition **default**, Flash **4 MB / QIO / 80 MHz**, CPU **240 MHz**, Loop/Event-Core **1**, Debug **none**, vollständiges Löschen **aus**. In der Arduino IDE dieselben Optionen wählen und `ESP32_PrusaConnectCam_web/ESP32_PrusaConnectCam_web.ino` öffnen. Das Skript erzeugt BIN-Dateien, `manifest.json` und `SHA256SUMS` im bestehenden Build-Verzeichnis. Große ELF/MAP-Zwischendateien bleiben unter `.build/`. Der Export aktualisiert auch Version, Dateigrößen und Prüfsummen des Web-Flashers.

Das M5Stack-Paket 3.2.2 meldet intern Arduino-Core 3.2.1; das ist erwartbar. Es erzeugt Warnungen über mehrfach definierte Pin-Makros in seinen eigenen Headern. Diese stammen aus dem unveränderten Herstellerpaket. Nicht auf ein anderes Board-Paket wechseln, um diese Warnungen zu entfernen.

Die Tests benötigen Linux, GCC/G++ und Bash:

```bash
./tests/run.sh
```

Geprüft werden HTTP-Statusauswertung, beide QR-Link-Formate, fehlerhafte Eingaben, EEPROM-Grenzen, tatsächliche QR-Erkennung einschließlich Spiegelung und simulierte Speicherfehler mit AddressSanitizer/UndefinedBehaviorSanitizer. In Umgebungen ohne LeakSanitizer-Unterstützung: `ASAN_OPTIONS=detect_leaks=0 ./tests/run.sh`; die QR-Tests zählen zusätzlich offene Speicherallokationen.

**Validierungsgrenze:** Kompilierung und Softwaretests ersetzen keinen Test am Gerät mit gültiger Prusa-Kopplung. Die Hardwareprüfung umfasst Kaltstart, aktualisierte Bilder, Netzwerkausfall/-wiederkehr, QR-Kopplung und erhaltene Einstellungen nach Neustart.

## Umfang und Quellen

Diese Firmware bietet Snapshots über Ethernet. Ein lokales Webinterface, RTSP, MQTT und OTA sind nicht implementiert; der historische Sketchname mit `_web` bedeutet kein Webinterface.

- [Prusa Camera API](https://connect.prusa3d.com/docs/cameras/) und [Token/Fingerprint-Kommunikation](https://connect.prusa3d.com/docs/cameras/camera_communication/)
- [Öffentliche Prusa-Webcam-Anwendung](https://camera-service-webcam.prusa3d.com/) – API-Konfiguration und QR-Link-Format, Stand 03.10.2026
- [Google Trust Services Root-Zertifikate](https://pki.goog/repository/) und [ISRG-Zertifikate](https://letsencrypt.org/certificates/)
- [M5Stack Unit PoE CAM](https://docs.m5stack.com/en/unit/Unit_PoE_CAM) und [Arduino-Anleitung](https://docs.m5stack.com/en/arduino/m5poe_cam/program)

Projekt: Markus Voth. Projektlizenz: MIT. Die mitgelieferten `quirc`- und OpenMV-Dateien tragen eigene Lizenz- und Copyright-Hinweise in ihren Quelltexten; diese bleiben erhalten.
