# PrusaConnectCam – M5Stack PoE-CAM

[English](README_en.md) · Firmware **1.4.0** · [Web-Flasher](https://vothmarkus.github.io/PrusaConnectCam-M5Stack-PoECam/)

Die **M5Stack Unit PoE CAM U121** (ESP32, W5500, OV2640) sendet JPEG-Schnappschüsse über Ethernet an Prusa Connect. Ein PoE-Switch oder PoE-Injector versorgt sie mit Strom. Standard: ein Bild alle 10 Sekunden, 1600 × 1200 Pixel, JPEG-Qualitätswert 20.

**Neu in 1.4.0:** nächtliche Firmware-Updates aus veröffentlichten GitHub-Releases, geprüfte HTTPS-Downloads, Start-Selbsttest und Bootloader-Rollback. Das funktionierende **M5Stack-Boardpaket 3.2.2**, Kamera-Pins und Partitionslayout bleiben die Grundlage. Der ursprüngliche Bootloader ist byteidentisch.

**Stand der Prüfung:** Build und Softwaretests sind erfolgreich. Die bisherige Firmware 1.3.1 wurde am PC und am Android-Handy erfolgreich geflasht, laut Rückmeldung auch mit höherer Baudrate. Der erste OTA-Wechsel und der Rollback müssen noch an einer echten Kamera geprüft werden. Vor dem Einsatz an allen Kameras zuerst ein Gerät testen.

## Erste Installation der OTA-Funktion

1. Kamera mit einem **externen ESP32 Downloader und passendem PoE-CAM-Adapter** verbinden. Ein USB-Kabel allein ist kein Programmer. Siehe [M5Stack-Anleitung](https://docs.m5stack.com/en/unit/Unit_PoE_CAM).
2. Den [Web-Flasher](https://vothmarkus.github.io/PrusaConnectCam-M5Stack-PoECam/) direkt in Chrome/Edge am PC oder Chrome auf Android öffnen.
3. **„Kamera auf 1.4.0 aktualisieren“** verwenden. Die gespeicherte Prusa-Kopplung bleibt erhalten.
4. Kamera anschließend am PoE-Netz betreiben. Im seriellen Protokoll erscheinen `M5PoECAM Prusa Connect 1.4.0`, `Camera ready` und nach mindestens 30 Sekunden `OTA self-test passed`.
5. Einen neuen Schnappschuss in Prusa Connect prüfen.

Diese erste OTA-fähige Firmware muss einmal per USB auf jede Kamera. Danach erfolgen normale Firmware-Updates über Ethernet. Alte Versionen bis 1.3.1 können sich nicht selbst aktualisieren.

**Vollständige Neuinstallation** im Flasher löscht den Flash einschließlich Kopplung. Sie ist für ein normales Update nicht erforderlich.

### Android / USB

Auf Android **„Android USB (CH9102)“** auswählen. Der WebUSB-Zugang unterstützt den M5Stack-Downloader mit USB-ID **`1a86:55d4`**. Ältere CP2104-Downloader bitte am PC verwenden.

- Seite direkt in Chrome über HTTPS öffnen, außerhalb eines eingebetteten App-Browsers.
- USB-Host/OTG-Adapter und ein Datenkabel verwenden. Ein reiner Steckeradapter garantiert keine Host-Funktion.
- **„Downloader erkennen“** auswählen und dem Gerät `USB-Enhanced-SERIAL CH9102` Zugriff erlauben. Dieser Test schreibt keine Firmware.
- Auf Android werden zunächst 115200 Baud gewählt; 460800 kann bei stabiler Verbindung verwendet werden.
- Eine Auswahl ausschließlich mit Bluetooth-Geräten deutet auf den nativen Serial-Modus oder eine alte Seitenversion hin. Ob `navigator.serial` vorhanden ist, sagt auf Android nichts über USB-Unterstützung aus ([Chromium](https://groups.google.com/a/chromium.org/g/blink-dev/c/HBJ-uYFvkpM/m/MrLnwZlsAAAJ)).

## Automatische Updates

Jede Kamera prüft **einmal pro Nacht zwischen 03:00 und 04:00 Uhr, Europe/Berlin**, einschließlich Sommerzeit. Aus der Geräte-MAC wird eine feste Minute/Sekunde berechnet, sodass die Geräte zeitversetzt anfragen. Die Uhrzeit und der letzte Prüftag stehen im Startprotokoll und unter `ota status`.

Voraussetzungen sind eine Ethernet-IP und eine gültige NTP-Zeit. Ist die Kamera zur geplanten Uhrzeit offline, versucht sie es nach Wiederkehr noch innerhalb derselben Stunde. Nach 04:00 Uhr wartet sie bis zur nächsten Nacht. Ein begonnener nächtlicher Versuch wird gespeichert; ein Neustart löst keine Wiederholung am selben Tag aus. Bei Download-, DNS- oder TLS-Fehlern läuft die bisherige Firmware weiter; der nächste automatische Versuch erfolgt in der nächsten Nacht.

Der Ablauf:

1. `https://github.com/vothmarkus/PrusaConnectCam-M5Stack-PoECam/releases/latest/download/ota-manifest.json` abrufen. GitHubs `latest` verweist auf einen veröffentlichten regulären Release; Entwürfe und Vorabversionen werden nicht verwendet.
2. Schema, Hardwareziel, Partitionslayout und eine **streng höhere Version** im Format `major.minor.patch` prüfen. Ein älterer Release führt niemals zu einem Downgrade.
3. Nur die App-BIN des angegebenen Releases herunterladen. Die Dateigröße muss in den freien OTA-Platz passen. HTTPS-Zertifikate werden mit dem CA-Bündel des SDK geprüft; jeder Redirect muss auf einen zugelassenen HTTPS-Host führen. Kamera-Token und Fingerprint werden nicht an GitHub gesendet.
4. In den inaktiven Firmware-Platz schreiben. **SHA-256 und ESP32-Image prüfen, bevor die Startauswahl geändert wird.** Das Merged-Image, die Partitionstabelle und der Einstellungsbereich werden bei OTA nicht geschrieben.
5. Neu starten, lokalen Selbsttest durchführen und die Version bestätigen. Während Download/Neustart pausieren die Schnappschüsse.

Ein Commit auf `main` installiert noch kein Update auf Kameras. **Das Veröffentlichen eines Releases ist die Freigabe.** Bis ein Release mit `ota-manifest.json` existiert, kann die Prüfung HTTP 404 melden; die Kamera arbeitet weiter. Der frühere Release `V1.2.0` enthält noch kein OTA-Manifest.

### Startprüfung und Rückfall

Eine neue OTA-Version wird erst nach PSRAM-Prüfung, erfolgreicher JPEG-Aufnahme, erfolgreichem Start des Ethernet-Treibers und mindestens 30 Sekunden Laufzeit bestätigt. Der Starttest ist bewusst unabhängig von NTP, Prusa Connect und Internet-Erreichbarkeit. Er prüft nicht jede mögliche Funktionsstörung nach dem Start; die tatsächlichen Prusa-Schnappschüsse gehören deshalb zum Gerätetest.

Scheitert der Test, kehrt die Kamera zur vorherigen Firmware zurück. Bei einem unbestätigten Neustart greift der Bootloader-Rollback. Für einen hängen gebliebenen Start ist zusätzlich ein Neustart nach 120 Sekunden vorgesehen. Eine zurückgewiesene Version wird gespeichert und anschließend übersprungen; für eine korrigierte Firmware eine neue Versionsnummer verwenden.

Der erste USB-Start bestätigt seinen eigenen Firmware-Platz als Rückfallziel für spätere OTA-Updates. USB-Reparaturen und eine unterbrochene Erstinstallation haben nicht denselben Schutz wie das Schreiben in den inaktiven OTA-Platz. Bei Bedarf bleibt der Programmer als Wiederherstellungsweg verfügbar.

### Sofort prüfen / OTA ausschalten

Seriellen Monitor auf **115200 Baud** stellen und den Befehl mit **Zeilenumbruch** senden:

| Befehl | Wirkung |
| --- | --- |
| `ota status` | Firmwareversion, aktiv/inaktiv, nächtliche Uhrzeit, letzter Prüftag, zurückgewiesene Version |
| `ota check` | Einmalige Prüfung anfordern; wartet nötigenfalls auf Selbsttest, Ethernet und NTP |
| `ota off` | Nächtliche Updates dauerhaft deaktivieren |
| `ota on` | Nächtliche Updates wieder aktivieren |

`ota check` ist auch bei deaktivierten automatischen Updates möglich. Ein Neustart setzt `ota off` nicht zurück. Diese Befehle werden an die laufende Firmware gesendet, nicht im Download-Modus des ESP32. Der Web-Flasher enthält keinen seriellen Monitor.

### Release erstellen und testen

1. `SW_VERSION` in `mcu_cfg.h` erhöhen und `./tools/build.sh` ausführen. Immer alle erzeugten BIN-Dateien und Metadaten gemeinsam mit den Quellen committen.
2. Der Workflow **„Validate firmware and prepare release“** prüft Quellenstand, Tests, Images, Prüfsummen und USB-Flasher. Bei Erfolg legt er einen **Release-Entwurf `v<Version>`** mit den fertigen Dateien an bzw. aktualisiert einen bestehenden Entwurf. Bereits veröffentlichte Firmware-Dateien werden nicht ersetzt.
3. Neue Firmware zunächst per USB an einer Kamera prüfen. Für den ersten echten OTA-Wechsel eine Kamera auf einer kleineren OTA-fähigen Version belassen, beispielsweise 1.4.0, und einen höheren Release vorbereiten.
4. Nach dem USB-Test den Entwurf tagsüber als normalen Release veröffentlichen und gegebenenfalls als **Latest** markieren. Mit `ota check` an der Testkamera den echten Download, Neustart und die Prusa-Bilder prüfen, bevor das nächste nächtliche Zeitfenster beginnt. Automatische Updates an übrigen bereits OTA-fähigen Kameras bei Bedarf vorher mit `ota off` deaktivieren.
5. Für den vollständigen Gerätetest auch den Wechsel zurück in den anderen OTA-Platz, einen unterbrochenen Download und einen gezielt fehlgeschlagenen Starttest an der Testkamera prüfen. Diesen fehlerhaften Test-Build nicht als regulären Release für die Flotte veröffentlichen.

Für einen Startfehler-Test kann das passende Test-Image mit Espressifs `otatool.py` in den inaktiven Platz geschrieben und dieser als nächster Start ausgewählt werden. Das erfordert Zugriff auf den Programmer und einen vorher dokumentierten funktionierenden Rückfallplatz. Einen Fehler nicht absichtlich an den produktiven Schulkameras auslösen.

## USB-Reparatur und Dateien

Die Hardware besitzt 16 MB Flash; das Projekt verwendet weiterhin das kompatible **4-MB-Layout**. Darin liegen `app0` und `app1` mit jeweils **1.280 KiB**. Die App 1.4.0 benötigt rund 983 KiB.

Alle [Firmware-Dateien, Prüfsummen und Metadaten](ESP32_PrusaConnectCam_web/build/m5stack.esp32.m5stack_poe_cam/) liegen unter dem bisherigen Download-Pfad.

| Datei | Adresse / Verwendung |
| --- | --- |
| `ESP32_PrusaConnectCam_web.ino.bin` | App für OTA; bei USB nach `0x10000` |
| `ESP32_PrusaConnectCam_web.ino.bootloader.bin` | Originaler M5Stack-Bootloader nach `0x1000` |
| `ota-reset.bin` | 8 KiB `0xFF` nach `0xe000`, **erst nach erfolgreichem Schreiben der App und des Bootloaders** |
| `ESP32_PrusaConnectCam_web.ino.partitions.bin` | Bestehende Partitionstabelle bei `0x8000`; bei normalem Update unverändert |
| `ESP32_PrusaConnectCam_web.ino.merged.bin` | Vollständige Erstinstallation nach `0x0`; löscht Kopplung |
| `ota-manifest.json` | Version, Zielgerät, Layout, Größe, SHA-256 und Release-URL der App |

**Der Web-Flasher erledigt die USB-Schritte in der richtigen Reihenfolge.** Er prüft vorher alle Downloads, schreibt App und den originalen Bootloader mit `eraseAll: false` und setzt zuletzt nur die beiden OTA-Metadatensektoren zurück. Der Kopplungsbereich `0x9000–0xdfff` wird nicht berührt.

Nur eine App nach `0x10000` zu schreiben genügt nach einem OTA-Wechsel möglicherweise nicht: Der Bootloader könnte weiterhin `app1` starten. Deshalb gehört das abschließende Schreiben von `ota-reset.bin` zur USB-Reparatur. Niemals diesen Schritt ausführen, wenn das Schreiben der neuen App fehlgeschlagen ist. Die Anleitung gilt für das Partitionslayout dieses Projekts, nicht für beliebige Fremdfirmware.

## Koppeln und bedienen

1. In Prusa Connect beim Drucker eine externe Kamera hinzufügen und den Kopplungs-QR-Code anzeigen.
2. Seitentaste der PoE-CAM kurz drücken und loslassen. Die LED blinkt schnell; der Scan läuft maximal 30 Sekunden.
3. QR-Code gut beleuchtet und vollständig ins Bild halten. Bei Erfolg wird der Token gespeichert; anschließend einen neuen Schnappschuss prüfen.

Ein weiterer kurzer Tastendruck bricht den Scan ab und erhält die bisherige Kopplung. Unterstützt werden reine 20-stellige alphanumerische Token, alte `?token=…`-Links und neue `#t=v1.…`-QR-Codes.

**Taste 5 Sekunden halten:** horizontale Spiegelung ändern, speichern, neu starten. Das ist kein Werksreset. Jede Kamera benötigt ihren eigenen Token; keinen vollständigen Flash-Abzug einer gekoppelten Kamera auf andere Geräte kopieren.

## LED und Fehlersuche

| Wiederholtes Blinken | Zustand |
| --- | --- |
| 1 Sekunde an / aus | Ethernet ohne IP |
| 0,25 Sekunden an / aus | NTP-Zeit fehlt |
| 2 Sekunden an / aus | Kein gültiger Kopplungs-Token |
| Schnell während des Scans | QR-Erkennung |
| Sehr schnell bei gehaltener Taste | Nach 5 Sekunden Spiegelung und Neustart |

| Einzelne Blinkfolge | Ergebnis |
| --- | --- |
| 1 × | Upload erfolgreich (HTTP 2xx), auch Bestätigung einer QR-Kopplung |
| 2 × | DNS-, TCP-, TLS- oder Übertragungsfehler |
| 3 × / 4 × | HTTP 401 / 403: Kopplung prüfen |
| 5 × / 6 × | Ungültiger Token / Fingerprint |
| 7 × | Anderer HTTP-Fehler, z. B. 301, 404, 429 oder 5xx |
| 8 × | Kamera-Initialisierung oder Aufnahme fehlgeschlagen |

Das Schulnetz muss DHCP/DNS, NTP über **UDP 123** zu `pool.ntp.org` oder `time.nist.gov` sowie HTTPS über **TCP 443** zu `camera-service.prusa3d.com` erlauben. OTA benötigt zusätzlich `github.com`, `release-assets.githubusercontent.com` und gegebenenfalls `objects.githubusercontent.com`. Captive Portals und authentifizierte Proxys werden nicht unterstützt. Ein TLS-Proxy mit eigener CA benötigt eine passende Netzfreigabe bzw. eine bewusst angepasste Vertrauenskette; Zertifikatsprüfung nicht abschalten.

**Prusa-Ausfall im Oktober 2026:** Der alte Upload-Endpunkt `webcam.connect.prusa3d.com/c/snapshot` lieferte eine 301-Weiterleitung. Verwendet wird jetzt `https://camera-service.prusa3d.com/c/snapshot`. Uploads prüfen HTTP-Status, begrenzen Wartezeiten und folgen keinen Weiterleitungen mit Kamera-Zugangsdaten. Zum bisherigen ISRG Root X1 wurden GTS Root R1/R4 ergänzt; ISRG Root X1 war nicht abgelaufen.

**Kamera-Startfehler in 1.3.0:** Ein versehentlicher Wechsel von M5Stack 3.2.2 auf Espressif 3.2.0 führte zu einem anderen SDK. 1.3.1 stellte das ursprüngliche Paket wieder her; danach wurde der Betrieb bestätigt. Achtmaliges Blinken und `Camera probe failed ... 0x105` betreffen den Sensor und treten vor HTTPS auf. 1.4.0 verwendet weiterhin den wiederhergestellten Stand mit ESP-IDF `v5.4.2-25-g858a988d6e`.

## Selbst bauen und Softwaretests

Referenz: **Arduino CLI 1.3.1**, **M5Stack 3.2.2**, Arduino-Core 3.2.1, ESP-IDF `v5.4.2-25-g858a988d6e`. Keine zusätzlichen Arduino-Bibliotheken erforderlich; JSON, HTTPS/CA-Bündel und OTA stammen aus dem SDK, `quirc` liegt im Projekt.

```bash
git clone https://github.com/vothmarkus/PrusaConnectCam-M5Stack-PoECam.git
cd PrusaConnectCam-M5Stack-PoECam
arduino-cli core update-index --additional-urls https://static-cdn.m5stack.com/resource/arduino/package_m5stack_index.json
arduino-cli core install m5stack:esp32@3.2.2 --additional-urls https://static-cdn.m5stack.com/resource/arduino/package_m5stack_index.json
./tools/build.sh
```

Das Skript setzt PSRAM **enabled**, Partition **default**, Flash **4 MB / QIO / 80 MHz**, CPU **240 MHz**, Loop/Event-Core **1**, Debug **none**, vollständiges Löschen **aus**. Es prüft SDK, Original-Bootloader, Partitionshash, App-Größe und den tatsächlich verlinkten Arduino-Rollback-Hook. Danach erzeugt es alle Images und Metadaten und aktualisiert den Web-Flasher. ELF/MAP bleiben unter `.build/`.

Das Herstellerpaket meldet intern Arduino-Core 3.2.1 und verursacht Warnungen über mehrfach definierte Pin-Makros. Dafür das Boardpaket nicht wechseln.

Linux-Testvoraussetzungen: GCC/G++, Bash, OpenSSL-Entwicklungsdateien (`libssl-dev`), Python 3 und Node.js ≥22.

```bash
./tests/run.sh
node --test tests/web_flasher_test.mjs
python3 tools/check_release.py
```

Die Tests prüfen HTTP/QR/EEPROM, echte QR-Erkennung und Speicherfehler, OTA-Versionsvergleich, Nachtzeitfenster, Redirect-Ziele, den tatsächlich verwendeten Streaming-Code einschließlich SHA-256, Abbrüchen und Schreibfehlern sowie Android-USB und die Reihenfolge der USB-Wiederherstellung. AddressSanitizer/UndefinedBehaviorSanitizer sind aktiv. Falls LeakSanitizer in der Umgebung nicht unterstützt wird: `ASAN_OPTIONS=detect_leaks=0 ./tests/run.sh`.

Die Prüfungen simulieren keine komplette ESP32-Hardware. Kaltstart, reale OTA-Übertragung, beide OTA-Plätze, Rollback, Prusa-Uploads und erhaltene Kopplung gehören zusätzlich zum Gerätetest.

## Quellen und Lizenz

- [Prusa Camera API](https://connect.prusa3d.com/docs/cameras/)
- [ESP-IDF 5.4.2: OTA und Rollback](https://docs.espressif.com/projects/esp-idf/en/v5.4.2/esp32/api-reference/system/ota.html)
- [GitHub Releases](https://docs.github.com/en/rest/releases/releases)
- [M5Stack Unit PoE CAM](https://docs.m5stack.com/en/unit/Unit_PoE_CAM)
- [Google Trust Services](https://pki.goog/repository/) / [ISRG-Zertifikate](https://letsencrypt.org/certificates/)

Projekt: Markus Voth, MIT-Lizenz. `quirc`, OpenMV-Dateien und die unverändert eingebundene Google-Web-Serial-Bibliothek behalten ihre eigenen Lizenzhinweise. Die Firmware bietet Ethernet-Snapshots und OTA; ein lokales Webinterface, RTSP und MQTT sind nicht implementiert.
