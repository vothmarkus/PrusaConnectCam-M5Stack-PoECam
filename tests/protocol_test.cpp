#include "../ESP32_PrusaConnectCam_web/protocol.h"
#include "qr_fixtures.h"
#include <cassert>
#include <cstring>
#include <iostream>
#include <string>
static void parse(const std::string &input, bool expected) {
  char token[21] = "unchanged";
  assert(prusa::pairingToken(input.data(), input.size(), token) == expected);
  assert(std::strcmp(token, expected ? expected_token : "unchanged") == 0);
}
int main() {
  using prusa::UploadResult;
  for (int status : {200, 201, 204, 299}) assert(prusa::uploadResult(status) == UploadResult::Success);
  for (int status : {301, 302, 307, 308, 400, 404, 429, 500, 503})
    assert(prusa::uploadResult(status) == UploadResult::Http);
  assert(prusa::uploadResult(401) == UploadResult::Unauthorized);
  assert(prusa::uploadResult(403) == UploadResult::Forbidden);
  for (int status : {0, -1, -11}) assert(prusa::uploadResult(status) == UploadResult::Connection);
  parse(expected_token, true);
  parse(std::string(" \r\n") + expected_token + "\t ", true);
  parse(legacy_payload, true);
  parse(current_payload, true);
  parse(std::string(current_payload) + "=", true);
  parse(std::string(current_payload) + "&lang=de", true);
  parse(std::string("https://example.test/?lang=de&token=") + expected_token + "#preview", true);
  parse("https://example.test/?token=%41bCdEfGhIj0123456789&lang=de", true);
  parse(std::string("https://example.test/#t=v2.") + (encoded_token + 3), false);
  parse("", false);
  parse("AbCdEfGhIj012345678", false);
  parse("AbCdEfGhIj01234567890", false);
  parse("AbCdEfGhIj012345678!", false);
  parse(std::string("https://example.test/?not_token=") + expected_token, false);
  parse(std::string("https://example.test/#token=") + expected_token, false);
  parse(std::string("https://example.test/?token=") + expected_token + "extra", false);
  parse("https://example.test/?token=AbCdEfGhIj01234567%0A", false);
  parse("https://example.test/?token=AbCdEfGhIj01234567%00", false);
  parse("https://example.test/?token=AbCdEfGhIj01234567%xx", false);
  parse("https://example.test/?token=AbCdEfGhIj012345678%", false);
  parse(std::string("https://example.test/?token=") + std::string(5000, 'A'), false);
  std::string bad = current_payload; bad.back() = '!'; parse(bad, false);
  bad = expected_token; bad[3] = '\0'; parse(bad, false);
  char out[21];
  assert(!prusa::pairingToken(nullptr, 0, out));
  assert(!prusa::pairingToken(expected_token, 20, nullptr));
  assert(!prusa::validStoredLength(0, 40));
  assert(prusa::validStoredLength(39, 40));
  assert(!prusa::validStoredLength(40, 40));
  assert(prusa::validStoredLength(79, 80));
  assert(!prusa::validStoredLength(80, 80));
  assert(!prusa::validStoredLength(255, 80));
  assert(prusa::validHeaderValue("YWJjZA==", 8, 80));
  assert(!prusa::validHeaderValue("abc\r\nx", 6, 80));
  assert(!prusa::validHeaderValue("a b", 3, 80));
  assert(!prusa::validHeaderValue("", 0, 80));
  std::cout << "Protocol tests passed: HTTP, QR formats, malformed input, EEPROM/header bounds\n";
}
