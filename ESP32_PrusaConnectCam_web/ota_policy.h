#pragma once

#include <stddef.h>
#include <stdint.h>
#include <string.h>

namespace ota {
constexpr const char *Target = "m5stack-poe-cam-u121";
constexpr const char *Layout = "default-4m-ota-1280k-v1";
constexpr const char *Repository = "https://github.com/vothmarkus/PrusaConnectCam-M5Stack-PoECam";
constexpr const char *Asset = "ESP32_PrusaConnectCam_web.ino.bin";
constexpr size_t SlotSize = 0x140000;

struct Version { uint16_t part[3] = {}; };
// Only stable major.minor.patch versions; no prereleases, signs or overflow.
inline bool parseVersion(const char *text, Version &out) {
  if (!text) return false;
  for (unsigned i = 0; i < 3; ++i) {
    if (*text < '0' || *text > '9') return false;
    const bool zero = *text == '0';
    uint32_t value = 0;
    unsigned digits = 0;
    while (*text >= '0' && *text <= '9') {
      if (++digits > 5 || (zero && digits > 1)) return false;
      value = value * 10 + (*text++ - '0');
      if (value > 65535) return false;
    }
    out.part[i] = static_cast<uint16_t>(value);
    if (i < 2 && *text++ != '.') return false;
  }
  return *text == '\0';
}
inline bool newer(const char *candidate, const char *current) {
  Version a, b;
  if (!parseVersion(candidate, a) || !parseVersion(current, b)) return false;
  for (unsigned i = 0; i < 3; ++i) {
    if (a.part[i] != b.part[i]) return a.part[i] > b.part[i];
  }
  return false;
}
inline bool validHash(const char *text) {
  if (!text || strlen(text) != 64) return false;
  for (unsigned i = 0; i < 64; ++i)
    if (!((text[i] >= '0' && text[i] <= '9') || (text[i] >= 'a' && text[i] <= 'f'))) return false;
  return true;
}
inline bool allowedUrl(const char *url) {
  if (!url || strncmp(url, "https://", 8) != 0) return false;
  for (const char *p = url; *p; ++p)
    if (static_cast<unsigned char>(*p) <= 32 || *p == '\\' || *p == '#' || *p == '@') return false;
  const char *host = url + 8;
  const char *end = strchr(host, '/');
  if (!end) return false;
  const char *hosts[] = {"github.com", "release-assets.githubusercontent.com", "objects.githubusercontent.com"};
  for (const char *allowed : hosts)
    if (static_cast<size_t>(end - host) == strlen(allowed) && strncmp(host, allowed, end - host) == 0) return true;
  return false;
}
inline bool nightlyDue(int hour, unsigned secondOfHour, unsigned jitter, uint32_t day, uint32_t lastDay) {
  return hour == 3 && secondOfHour >= jitter && day > lastDay;
}
inline bool validImageSize(size_t size, size_t slot) { return size >= 288 && size <= slot && size <= SlotSize; }

// Two-phase transfer: verification finishes before the caller may select a
// boot partition. Both backends and the exact streaming logic are host-tested.
struct ImageWriter {
  virtual bool begin(size_t size) = 0;
  virtual bool write(const uint8_t *data, size_t size) = 0;
  virtual bool finish() = 0;
  virtual void abort() = 0;
  virtual ~ImageWriter() = default;
};
struct ImageHash {
  virtual bool update(const uint8_t *data, size_t size) = 0;
  virtual bool finish(uint8_t out[32]) = 0;
  virtual ~ImageHash() = default;
};
class Transfer {
 public:
  Transfer(ImageWriter &writer, ImageHash &hash, size_t size, const char *digest)
      : writer_(writer), hash_(hash), size_(size), digest_(digest) {}
  ~Transfer() { if (started_ && !finished_) writer_.abort(); }
  bool feed(const uint8_t *data, size_t size) {
    if (failed_ || finished_ || !validImageSize(size_, SlotSize) || !validHash(digest_) || size > size_ - received_)
      return fail();
    if (!size) return true;
    if (!hash_.update(data, size)) return fail();
    received_ += size;
    while (headerSize_ < sizeof(header_) && size) { header_[headerSize_++] = *data++; --size; }
    if (headerSize_ < sizeof(header_)) return true;
    if (!started_) {
      // ESP32 image, not a merged image / ESP32-S3 / other device family.
      if (header_[0] != 0xe9 || header_[12] != 0 || header_[13] != 0) return fail();
      if (!writer_.begin(size_)) return fail();
      started_ = true;
      if (!writer_.write(header_, sizeof(header_))) return fail();
    }
    return size == 0 || writer_.write(data, size) || fail();
  }
  bool finish() {
    if (failed_ || finished_ || !started_ || received_ != size_) return fail();
    uint8_t digest[32];
    if (!hash_.finish(digest)) return fail();
    constexpr char hex[] = "0123456789abcdef";
    for (unsigned i = 0; i < 32; ++i)
      if (digest_[2*i] != hex[digest[i] >> 4] || digest_[2*i+1] != hex[digest[i] & 15]) return fail();
    if (!writer_.finish()) return fail();
    finished_ = true;
    return true;
  }
 private:
  bool fail() { failed_ = true; return false; }
  ImageWriter &writer_;
  ImageHash &hash_;
  size_t size_, received_ = 0, headerSize_ = 0;
  const char *digest_;
  uint8_t header_[24] = {};
  bool started_ = false, finished_ = false, failed_ = false;
};
}  // namespace ota
