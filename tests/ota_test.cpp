#include "../ESP32_PrusaConnectCam_web/ota_policy.h"
#include <openssl/evp.h>
#include <algorithm>
#include <array>
#include <cassert>
#include <cstdio>
#include <string>
#include <vector>

using Bytes = std::vector<uint8_t>;
struct Hash : ota::ImageHash {
  EVP_MD_CTX *ctx = EVP_MD_CTX_new();
  Hash() { assert(ctx && EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr)); }
  ~Hash() override { EVP_MD_CTX_free(ctx); }
  bool update(const uint8_t *data, size_t length) override { return EVP_DigestUpdate(ctx, data, length) == 1; }
  bool finish(uint8_t out[32]) override { return EVP_DigestFinal_ex(ctx, out, nullptr) == 1; }
};
std::string sha256(const Bytes &bytes) {
  Hash h; uint8_t digest[32]; assert(h.update(bytes.data(), bytes.size())); assert(h.finish(digest));
  std::string out; const char hex[] = "0123456789abcdef";
  for (auto b : digest) { out += hex[b >> 4]; out += hex[b & 15]; }
  return out;
}
struct Writer : ota::ImageWriter {
  Bytes written;
  unsigned begins = 0, finishes = 0, aborts = 0;
  bool failBegin = false, failWrite = false, failFinish = false;
  bool begin(size_t) override { ++begins; return !failBegin; }
  bool write(const uint8_t *data, size_t size) override {
    if (failWrite) return false;
    written.insert(written.end(), data, data + size); return true;
  }
  bool finish() override { ++finishes; return !failFinish; }
  void abort() override { ++aborts; }
};
int main() {
  ota::Version parsed;
  for (const char *s : {"", "v1.4.0", "1.4", "1.4.0-rc1", "1.4.0\n", "01.4.0", "1..0", "-1.4.0", "65536.0.0", "1.4.0.1"})
    assert(!ota::parseVersion(s, parsed));
  assert(ota::newer("1.10.0", "1.9.9"));
  assert(ota::newer("2.0.0", "1.99.99"));
  assert(!ota::newer("1.2.0", "1.4.0"));
  assert(!ota::newer("1.4.0", "1.4.0"));
  assert(!ota::newer("1.5.0-beta", "1.4.0"));
  assert(ota::parseVersion("65535.65535.65535", parsed));
  assert(ota::allowedUrl("https://github.com/owner/repo/releases/latest/download/file"));
  assert(ota::allowedUrl("https://release-assets.githubusercontent.com/path?token=example&x=1"));
  for (const char *s : {"http://github.com/path", "https://github.com.evil.test/path", "https://github.com@evil.test/path", "https://evil.test/github.com/path", "https://github.com:80/path", "https://github.com\\evil/path", "https://github.com/path\r\nHeader: injected", "//github.com/path"})
    assert(!ota::allowedUrl(s));
  assert(!ota::validImageSize(0, ota::SlotSize));
  assert(!ota::validImageSize(ota::SlotSize + 1, ota::SlotSize));
  assert(!ota::validImageSize(1000, 999));
  assert(ota::validImageSize(ota::SlotSize, ota::SlotSize));
  assert(ota::nightlyDue(3, 120, 120, 20261006, 20261005));
  assert(!ota::nightlyDue(3, 119, 120, 20261006, 20261005));
  assert(!ota::nightlyDue(4, 120, 120, 20261006, 20261005));
  assert(!ota::nightlyDue(3, 120, 120, 20261006, 20261006));
  assert(!ota::nightlyDue(3, 120, 120, 20261005, 20261006));

  Bytes bytes(2049);
  for (size_t i = 0; i < bytes.size(); ++i) bytes[i] = i & 255;
  bytes[0] = 0xe9; bytes[12] = 0; bytes[13] = 0;
  const std::string digest = sha256(bytes);
  for (size_t chunk : {1u, 7u, 23u, 24u, 25u, 1024u, 2049u}) {
    Writer writer; Hash hash;
    {
      ota::Transfer transfer(writer, hash, bytes.size(), digest.c_str());
      for (size_t i = 0; i < bytes.size(); i += chunk)
        assert(transfer.feed(bytes.data() + i, std::min(chunk, bytes.size() - i)));
      assert(transfer.finish()); assert(!transfer.finish());
      assert(writer.written == bytes && writer.finishes == 1);
    }
    assert(writer.aborts == 0);
  }
  // Every truncation, including power loss midway through the header/body.
  for (size_t size = 0; size < bytes.size(); ++size) {
    Writer writer; Hash hash;
    { ota::Transfer transfer(writer, hash, bytes.size(), digest.c_str());
      assert(transfer.feed(bytes.data(), size)); assert(!transfer.finish()); }
    assert(writer.finishes == 0);
    assert(writer.aborts == (size >= 24 ? 1u : 0u));
  }
  for (int scenario = 0; scenario < 8; ++scenario) {
    Writer writer; Hash hash; Bytes bad = bytes;
    std::string expected = digest;
    if (scenario == 0) bad[0] = 0xff;
    if (scenario == 1) bad[12] = 9; // ESP32-S3
    if (scenario == 2) bad[100] ^= 1;
    if (scenario == 3) bad.push_back(0);
    if (scenario == 4) writer.failBegin = true;
    if (scenario == 5) writer.failWrite = true;
    if (scenario == 6) writer.failFinish = true;
    if (scenario == 7) expected[0] = 'x';
    { ota::Transfer transfer(writer, hash, bytes.size(), expected.c_str());
      const bool fed = transfer.feed(bad.data(), bad.size());
      assert(!fed || !transfer.finish()); }
    if (scenario != 6) assert(writer.finishes == 0);
  }
  std::puts("OTA policy and streaming: versions, nightly schedule, HTTPS redirects, SHA-256, truncation and flash failures passed");
}
