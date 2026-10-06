/*
 * Prusa Connect camera for M5Stack Unit PoE CAM (ESP32 + W5500).
 * Adapted by Markus Voth. See README.md for wiring, flashing and LED codes.
 */
#include <Ticker.h>
#include <time.h>
#include "Arduino.h"
#include "esp_arduino_version.h"
#include "server.h"
#include "cfg.h"
#include "var.h"
#include "mcu_cfg.h"
#include "qr.h"
#include "ota.h"

Ticker blinker;
static volatile bool ethConnected = false;
static volatile uint8_t blinkEdges = 0;
static uint8_t ledMode = 255;
static bool ntpStarted = false;

// Active-low LED. Only mode changes replace the timer, preserving error pulses.
void blinking() { digitalWrite(LED_PIN, !digitalRead(LED_PIN)); }
void blinkTimes() {
  if (blinkEdges) {
    blinking();
    blinkEdges = blinkEdges - 1;
  }
  if (!blinkEdges) {
    blinker.detach();
    digitalWrite(LED_PIN, HIGH);
  }
}
void setLedMode(uint8_t mode) {
  if (mode == ledMode) return;
  ledMode = mode;
  blinker.detach();
  blinkEdges = 0;
  digitalWrite(LED_PIN, HIGH);
  switch (mode) {
    case 1: blinker.attach(1.0, blinking); break; // no Ethernet IP
    case 2: blinker.attach(0.25, blinking); break; // waiting for time
    case 3: blinker.attach(2.0, blinking); break; // not paired
    case 4: blinker.attach(0.15, blinking); break; // QR scanning
    case 5: blinker.attach(0.1, blinking); break; // button held
    default: break;
  }
}
void signalResult(uint8_t result) {
  setLedMode(0);
  blinker.detach();
  blinkEdges = 2 * (result + 1) - 1;
  digitalWrite(LED_PIN, LOW);
  blinker.attach(result ? 0.25 : 0.5, blinkTimes);
}
void GPIO_Init() {
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);
  pinMode(FLASH_PIN, OUTPUT);
  digitalWrite(FLASH_PIN, LOW);
  pinMode(BUTTON_PIN, INPUT);
}
// Short release starts/cancels scanning. Holding 5 s keeps the original
// horizontal-mirror + reboot function; pairing is not erased.
bool shortButtonPress() {
  if (digitalRead(BUTTON_PIN)) return false;
  delay(40);
  if (digitalRead(BUTTON_PIN)) return false;
  const uint32_t pressedAt = millis();
  setLedMode(5);
  while (!digitalRead(BUTTON_PIN)) {
    if (static_cast<uint32_t>(millis() - pressedAt) >= 5000) {
      Cfg_ToggleHmirror();
      ESP.restart();
    }
    delay(5);
  }
  delay(40);
  return true;
}
void onEvent(arduino_event_id_t event, arduino_event_info_t info) {
  (void)info;
  switch (event) {
    case ARDUINO_EVENT_ETH_START:
      ETH.setHostname(EthernetDeciveName.c_str());
      Serial.println("Ethernet started");
      break;
    case ARDUINO_EVENT_ETH_GOT_IP:
      ethConnected = true;
      Serial.print("Ethernet IP: ");
      Serial.println(ETH.localIP());
      break;
    case ARDUINO_EVENT_ETH_LOST_IP:
    case ARDUINO_EVENT_ETH_DISCONNECTED:
    case ARDUINO_EVENT_ETH_STOP:
      ethConnected = false;
      Serial.println("Ethernet disconnected / no IP");
      break;
    default: break;
  }
}
bool ETH_Init() {
  char hostname[24];
  const uint64_t mac = ESP.getEfuseMac();
  // getEfuseMac stores MAC bytes little-endian; use the device suffix.
  snprintf(hostname, sizeof(hostname), "prusaCAM-%02X%02X%02X",
           static_cast<unsigned>((mac >> 24) & 255),
           static_cast<unsigned>((mac >> 32) & 255),
           static_cast<unsigned>((mac >> 40) & 255));
  EthernetDeciveName = hostname;
  Network.onEvent(onEvent);
  SPI.begin(ETH_SPI_SCK, ETH_SPI_MISO, ETH_SPI_MOSI, ETH_PHY_CS);
  const bool ready = ETH.begin(ETH_PHY_TYPE, ETH_PHY_ADDR, ETH_PHY_CS, ETH_PHY_IRQ, ETH_PHY_RST, SPI);
  if (!ready)
    Serial.println("Ethernet initialization failed");
  return ready;
}
void scanQr() {
  Serial.println("QR scan started (30 s); short button press cancels");
  setLedMode(4);
  if (!Camera_Reinit(0, true)) {
    Camera_Reinit();
    signalResult(static_cast<uint8_t>(prusa::UploadResult::Camera));
    return;
  }
  const uint32_t startedAt = millis();
  bool paired = false;
  while (static_cast<uint32_t>(millis() - startedAt) < QR_TIMEOUT * 1000UL) {
    if (shortButtonPress()) break;
    if (Camera_CapturePhoto()) {
      const String token = qrCodeDetect();
      Camera_ReleasePhoto();
      if (!token.isEmpty() && Cfg_SaveToken(token)) {
        sToken = token;
        paired = true;
        break;
      }
    }
    delay(50);
  }
  Camera_ReleasePhoto();
  const bool restored = Camera_Reinit();
  Serial.println(paired ? "Pairing token saved" : "QR scan ended; previous pairing retained");
  if (!restored) signalResult(static_cast<uint8_t>(prusa::UploadResult::Camera));
  else if (paired) signalResult(0);
  else setLedMode(0);
}
void setup() {
  Serial.begin(SERIAL_PORT_SPEED);
  Serial.println("\nM5PoECAM Prusa Connect " SW_VERSION);
  Serial.printf("Board: %s | Arduino core: %s | ESP-IDF: %s | PSRAM: %u bytes\n",
                ARDUINO_BOARD, ESP_ARDUINO_VERSION_STR, ESP.getSdkVersion(),
                static_cast<unsigned>(ESP.getPsramSize()));
  GPIO_Init();
  Ota_Begin();
  Cfg_Init();
  const bool cameraReady = Camera_InitCamera();
  const bool ethernetReady = ETH_Init();
  Ota_SetHardwareHealth(cameraReady, ethernetReady);
}
SET_LOOP_TASK_STACK_SIZE(40 * 1024); // quirc structures use stack space
void loop() {
  static int64_t lastSlot = -1;
  static uint32_t lastAttempt = 0;
  static uint32_t lastNtpLog = 0;
  Ota_Loop(ethConnected, time(nullptr));
  if (shortButtonPress()) scanQr();
  if (!ethConnected) {
    setLedMode(1);
    delay(5);
    return;
  }
  if (!ntpStarted) {
    // Start asynchronously in loop, never block the Ethernet event task.
    configTzTime("CET-1CEST,M3.5.0,M10.5.0/3", "pool.ntp.org", "time.nist.gov");
    ntpStarted = true;
    Serial.println("NTP started; waiting for a valid clock before HTTPS");
  }
  const time_t now = time(nullptr);
  if (now < MIN_VALID_UNIX_TIME) {
    setLedMode(2);
    if (static_cast<uint32_t>(millis() - lastNtpLog) >= 30000) {
      lastNtpLog = millis();
      Serial.println("Still waiting for NTP; check DNS and UDP port 123");
    }
    delay(5);
    return;
  }
  if (sToken.isEmpty()) {
    setLedMode(3);
    delay(5);
    return;
  }
  setLedMode(0);
  const int64_t slot = static_cast<int64_t>(now) / RefreshInterval;
  // Epoch slots preserve NTP alignment; monotonic time prevents bursts after
  // clock adjustments. Unsigned millis subtraction handles the 49-day wrap.
  if (slot != lastSlot && (lastSlot < 0 ||
      static_cast<uint32_t>(millis() - lastAttempt) >= RefreshInterval * 1000UL)) {
    lastSlot = slot;
    lastAttempt = millis();
    uint8_t result = static_cast<uint8_t>(prusa::UploadResult::Camera);
    if (Camera_CapturePhoto()) result = Server_SendPhotoToPrusaBackend();
    Camera_ReleasePhoto();
    signalResult(result);
  }
  delay(5);
}
