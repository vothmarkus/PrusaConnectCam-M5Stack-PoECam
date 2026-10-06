#include "ota.h"
#include "ota_policy.h"
#include "mcu_cfg.h"
#include "camera.h"
#include <Preferences.h>
#include <cJSON.h>
#include <esp_crt_bundle.h>
#include <esp_http_client.h>
#include <esp_ota_ops.h>
#include <esp_timer.h>
#include <mbedtls/sha256.h>
#include <algorithm>

#if !CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE || !CONFIG_MBEDTLS_CERTIFICATE_BUNDLE
#error "OTA requires the original M5Stack SDK with rollback and the TLS certificate bundle"
#endif

// Override the Arduino weak default: do not confirm an OTA image before our
// camera/Ethernet self-test has run. This is called before setup().
extern "C" bool verifyRollbackLater() { return true; }

namespace {
constexpr char ManifestUrl[] = "https://github.com/vothmarkus/PrusaConnectCam-M5Stack-PoECam/releases/latest/download/ota-manifest.json";
constexpr uint32_t DownloadTimeoutMs = 180000;
Preferences settings;
bool settingsReady = false, enabled = true, pending = false, healthy = false;
bool hardwareReady = false, hardwareReported = false, requested = false;
uint32_t bootAt = 0, lastDay = 0;
unsigned jitter = 0;
String rejectedVersion;
esp_timer_handle_t bootDeadline = nullptr;

void stopBootDeadline() {
  if (bootDeadline) {
    esp_timer_stop(bootDeadline);
    esp_timer_delete(bootDeadline);
    bootDeadline = nullptr;
  }
}
void bootTimedOut(void *) {
  // The bootloader rejects an unconfirmed image on the next boot, including
  // if setup/loop hangs. No flash/NVS work from this timer callback.
  esp_restart();
}
void rejectBoot(const char *reason) {
  Serial.printf("OTA self-test failed: %s; reverting to the previous firmware\n", reason);
  Serial.flush();
  const esp_err_t error = esp_ota_mark_app_invalid_rollback_and_reboot();
  Serial.printf("OTA rollback unavailable: %s. USB recovery required.\n", esp_err_to_name(error));
  stopBootDeadline();
  pending = false;
  enabled = false;
}

// Bounded HTTPS GET with explicit HTTPS/host checks on every redirect. No
// camera token, fingerprint or GitHub credential is sent to this client.
class Download {
 public:
  ~Download() { close(); }
  int status = 0;
  int64_t length = -1;
  bool open(const char *initial) {
    String url(initial);
    for (unsigned redirects = 0; redirects <= 4; ++redirects) {
      close();
      if (!ota::allowedUrl(url.c_str())) return false;
      location_ = "";
      esp_http_client_config_t config = {};
      config.url = url.c_str();
      config.crt_bundle_attach = esp_crt_bundle_attach;
      config.timeout_ms = 15000;
      config.buffer_size = 1024;
      config.buffer_size_tx = 2048;
      config.user_agent = "M5PoECAM-PrusaConnect/" SW_VERSION;
      config.disable_auto_redirect = true;
      config.event_handler = event;
      config.user_data = this;
      client_ = esp_http_client_init(&config);
      if (!client_) return false;
      esp_http_client_set_header(client_, "Accept-Encoding", "identity");
      if (esp_http_client_open(client_, 0) != ESP_OK) return false;
      length = esp_http_client_fetch_headers(client_);
      status = esp_http_client_get_status_code(client_);
      if (status == 200) return length > 0;
      if (!(status == 301 || status == 302 || status == 303 || status == 307 || status == 308)) return false;
      if (location_.isEmpty() || location_.length() > 2048) return false;
      if (location_[0] == '/' && location_[1] != '/') {
        const int slash = url.indexOf('/', 8);
        url = url.substring(0, slash) + location_;
      } else url = location_;
    }
    return false;
  }
  int read(uint8_t *buffer, size_t length) {
    return esp_http_client_read(client_, reinterpret_cast<char *>(buffer), length);
  }
  bool complete() { return esp_http_client_is_complete_data_received(client_); }
  void close() {
    if (client_) { esp_http_client_cleanup(client_); client_ = nullptr; }
  }
 private:
  static esp_err_t event(esp_http_client_event_t *event) {
    if (event->event_id == HTTP_EVENT_ON_HEADER && event->header_key &&
        strcasecmp(event->header_key, "Location") == 0 && event->header_value) {
      auto *self = static_cast<Download *>(event->user_data);
      if (strlen(event->header_value) <= 2048) self->location_ = event->header_value;
    }
    return ESP_OK;
  }
  esp_http_client_handle_t client_ = nullptr;
  String location_;
};

struct Manifest { String version, url, sha256; size_t size = 0; };
const char *stringField(cJSON *json, const char *name) {
  const cJSON *field = cJSON_GetObjectItemCaseSensitive(json, name);
  return cJSON_IsString(field) && field->valuestring ? field->valuestring : "";
}
bool getManifest(Manifest &manifest) {
  Download download;
  if (!download.open(ManifestUrl)) {
    Serial.printf("OTA release manifest unavailable (HTTP %d); keeping current firmware\n", download.status);
    return false;
  }
  if (download.length > 2048) { Serial.println("OTA manifest too large"); return false; }
  char body[2049] = {};
  size_t used = 0;
  while (used < static_cast<size_t>(download.length)) {
    const int got = download.read(reinterpret_cast<uint8_t *>(body) + used, download.length - used);
    if (got <= 0) return false;
    used += got;
  }
  if (!download.complete()) return false;
  download.close();
  cJSON *json = cJSON_ParseWithLengthOpts(body, used + 1, nullptr, true);
  if (!json) { Serial.println("OTA manifest is not valid JSON"); return false; }
  const cJSON *schema = cJSON_GetObjectItemCaseSensitive(json, "schema");
  const cJSON *size = cJSON_GetObjectItemCaseSensitive(json, "size");
  manifest.version = stringField(json, "version");
  manifest.url = stringField(json, "url");
  manifest.sha256 = stringField(json, "sha256");
  ota::Version version;
  const bool valid = cJSON_IsNumber(schema) && schema->valuedouble == 1 &&
      cJSON_IsNumber(size) && size->valuedouble >= 288 && size->valuedouble <= ota::SlotSize &&
      size->valuedouble == static_cast<size_t>(size->valuedouble) &&
      strcmp(stringField(json, "target"), ota::Target) == 0 &&
      strcmp(stringField(json, "layout"), ota::Layout) == 0 &&
      strcmp(stringField(json, "file"), ota::Asset) == 0 &&
      ota::parseVersion(manifest.version.c_str(), version) && ota::validHash(manifest.sha256.c_str()) &&
      manifest.url == String(ota::Repository) + "/releases/download/v" + manifest.version + "/" + ota::Asset;
  if (valid) manifest.size = static_cast<size_t>(size->valuedouble);
  cJSON_Delete(json);
  if (!valid) Serial.println("OTA manifest rejected: schema, target, version, size, hash or URL mismatch");
  return valid;
}

class FlashWriter : public ota::ImageWriter {
 public:
  explicit FlashWriter(const esp_partition_t *partition) : partition_(partition) {}
  bool begin(size_t size) override {
    if (esp_ota_begin(partition_, size, &handle_) != ESP_OK) return false;
    active_ = true;
    return true;
  }
  bool write(const uint8_t *data, size_t size) override { return esp_ota_write(handle_, data, size) == ESP_OK; }
  bool finish() override {
    const esp_err_t error = esp_ota_end(handle_);
    active_ = false; // esp_ota_end frees the handle even on validation failure.
    return error == ESP_OK;
  }
  void abort() override { if (active_) esp_ota_abort(handle_); active_ = false; }
 private:
  const esp_partition_t *partition_;
  esp_ota_handle_t handle_ = 0;
  bool active_ = false;
};
class Sha256 : public ota::ImageHash {
 public:
  Sha256() { mbedtls_sha256_init(&context_); ok_ = mbedtls_sha256_starts(&context_, 0) == 0; }
  ~Sha256() { mbedtls_sha256_free(&context_); }
  bool update(const uint8_t *data, size_t size) override { return ok_ && mbedtls_sha256_update(&context_, data, size) == 0; }
  bool finish(uint8_t out[32]) override { return ok_ && mbedtls_sha256_finish(&context_, out) == 0; }
 private:
  mbedtls_sha256_context context_;
  bool ok_ = false;
};

bool install(const Manifest &manifest) {
  const esp_partition_t *partition = esp_ota_get_next_update_partition(nullptr);
  if (!partition || !ota::validImageSize(manifest.size, partition->size)) return false;
  Download download;
  if (!download.open(manifest.url.c_str()) || download.length != static_cast<int64_t>(manifest.size)) return false;
  FlashWriter writer(partition);
  Sha256 hash;
  ota::Transfer transfer(writer, hash, manifest.size, manifest.sha256.c_str());
  uint8_t buffer[1024];
  const uint32_t started = millis();
  size_t received = 0;
  while (received < manifest.size) {
    if (static_cast<uint32_t>(millis() - started) > DownloadTimeoutMs) return false;
    const int got = download.read(buffer, std::min(sizeof(buffer), manifest.size - received));
    if (got <= 0 || !transfer.feed(buffer, got)) return false;
    received += got;
    delay(1);
  }
  if (!download.complete() || !transfer.finish()) return false;
  download.close();
  // Record the trial BEFORE changing boot selection. After a rollback the old
  // app rejects this version, avoiding nightly install/rollback loops.
  if (!settings.putString("trial", manifest.version)) return false;
  if (esp_ota_set_boot_partition(partition) != ESP_OK) {
    settings.remove("trial");
    return false;
  }
  Serial.printf("OTA %s verified; restarting into %s\n", manifest.version.c_str(), partition->label);
  Serial.flush();
  esp_restart();
  return true;
}
void checkRelease() {
  Serial.println("OTA checking latest stable GitHub release");
  Manifest manifest;
  if (!getManifest(manifest)) return;
  if (!ota::newer(manifest.version.c_str(), SW_VERSION)) {
    Serial.printf("OTA current=%s release=%s; no newer firmware\n", SW_VERSION, manifest.version.c_str());
    return;
  }
  if (manifest.version == rejectedVersion) {
    Serial.printf("OTA %s previously failed its boot test; skipped\n", manifest.version.c_str());
    return;
  }
  Serial.printf("OTA downloading %s (%u bytes); snapshots paused during update\n", manifest.version.c_str(), static_cast<unsigned>(manifest.size));
  Camera_ReleasePhoto();
  if (!install(manifest)) Serial.println("OTA update aborted; current firmware and pairing retained");
}
void status() {
  Serial.printf("OTA %s | firmware %s | check at 03:%02u:%02u Europe/Berlin | last day %lu | rejected %s\n",
                enabled ? "enabled" : "disabled", SW_VERSION, jitter / 60, jitter % 60,
                static_cast<unsigned long>(lastDay), rejectedVersion.isEmpty() ? "none" : rejectedVersion.c_str());
}
void serialCommands() {
  static char command[32];
  static size_t length = 0;
  static bool overflow = false;
  while (Serial.available()) {
    const char c = Serial.read();
    if (c == '\r') continue;
    if (c != '\n') {
      if (length + 1 < sizeof(command)) command[length++] = c;
      else overflow = true;
      continue;
    }
    command[length] = '\0';
    if (!overflow) {
      if (strcmp(command, "ota status") == 0) status();
      else if (strcmp(command, "ota check") == 0) {
        requested = true;
        Serial.println("OTA manual check queued; waiting for self-test, Ethernet and NTP");
      } else if (strcmp(command, "ota on") == 0 || strcmp(command, "ota off") == 0) {
        const bool value = strcmp(command, "ota on") == 0;
        if (settingsReady && settings.putBool("enabled", value)) { enabled = value; status(); }
        else Serial.println("OTA setting could not be saved");
      } else if (length) Serial.println("Commands: ota status | ota check | ota on | ota off");
    }
    length = 0;
    overflow = false;
  }
}
}  // namespace

void Ota_Begin() {
  bootAt = millis();
  const uint64_t mac = ESP.getEfuseMac();
  jitter = static_cast<unsigned>((mac ^ (mac >> 24)) % 3600);
  esp_ota_img_states_t state;
  const esp_partition_t *running = esp_ota_get_running_partition();
  pending = running && esp_ota_get_state_partition(running, &state) == ESP_OK && state == ESP_OTA_IMG_PENDING_VERIFY;
  if (pending) {
    esp_timer_create_args_t timer = {};
    timer.callback = bootTimedOut;
    timer.name = "ota-boot-test";
    if (esp_timer_create(&timer, &bootDeadline) != ESP_OK || esp_timer_start_once(bootDeadline, 120000000) != ESP_OK) {
      rejectBoot("cannot start recovery timer");
      return;
    }
    Serial.println("OTA trial boot: self-test required within 120 seconds");
  }
  settingsReady = settings.begin("prusa-ota", false);
  if (!settingsReady) { enabled = false; Serial.println("OTA settings unavailable; automatic updates disabled"); return; }
  enabled = settings.getBool("enabled", true);
  lastDay = settings.getUInt("lastday", 0);
  rejectedVersion = settings.getString("rejected", "");
  const String trial = settings.getString("trial", "");
  if (!trial.isEmpty() && trial != SW_VERSION) {
    rejectedVersion = trial;
    if (settings.putString("rejected", trial)) settings.remove("trial");
    else enabled = false;
    Serial.printf("OTA previous trial %s did not remain active; blocked\n", trial.c_str());
  }
  status();
  Serial.println("OTA serial commands (115200 baud): ota status | ota check | ota on | ota off");
}
void Ota_SetHardwareHealth(bool cameraReady, bool ethernetReady) {
  hardwareReported = true;
  hardwareReady = cameraReady && ethernetReady && psramFound();
  if (hardwareReady) {
    hardwareReady = Camera_CapturePhoto() && photoFrame && photoFrame->format == PIXFORMAT_JPEG;
    Camera_ReleasePhoto();
  }
  if (pending && !hardwareReady) rejectBoot("PSRAM, camera capture or Ethernet driver");
}
void Ota_Loop(bool networkReady, time_t now) {
  serialCommands();
  if (!healthy && hardwareReported && hardwareReady && static_cast<uint32_t>(millis() - bootAt) >= 30000) {
    const esp_partition_t *running = esp_ota_get_running_partition();
    esp_ota_img_states_t state;
    esp_err_t error = esp_ota_get_state_partition(running, &state);
    // USB installations need a valid entry for app0 before their first OTA,
    // otherwise a power failure could leave no recorded rollback candidate.
    if (error != ESP_OK) error = esp_ota_set_boot_partition(running);
    if (error == ESP_OK) error = esp_ota_mark_app_valid_cancel_rollback();
    if (error != ESP_OK) {
      if (pending) rejectBoot("cannot confirm boot partition");
      else { hardwareReady = false; Serial.println("OTA boot validation failed; updates disabled"); }
      return;
    }
    stopBootDeadline();
    pending = false;
    healthy = true;
    if (settingsReady && settings.getString("trial", "") == SW_VERSION) settings.remove("trial");
    Serial.println("OTA self-test passed: PSRAM, JPEG capture, Ethernet driver and 30 s uptime");
  }
  if (!settingsReady || !healthy || !networkReady || now < MIN_VALID_UNIX_TIME) return;
  struct tm local;
  localtime_r(&now, &local);
  const uint32_t day = (local.tm_year + 1900) * 10000 + (local.tm_mon + 1) * 100 + local.tm_mday;
  const bool due = enabled && ota::nightlyDue(local.tm_hour, local.tm_min * 60 + local.tm_sec, jitter, day, lastDay);
  if (!requested && !due) return;
  requested = false;
  if (due) {
    if (!settings.putUInt("lastday", day)) { enabled = false; Serial.println("OTA cannot save check date; disabled"); return; }
    lastDay = day;
  }
  checkRelease();
}
