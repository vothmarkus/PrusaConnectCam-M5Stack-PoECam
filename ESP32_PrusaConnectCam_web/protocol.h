#ifndef PRUSA_PROTOCOL_H
#define PRUSA_PROTOCOL_H
#include <stddef.h>
#include <stdint.h>
#include <string.h>

namespace prusa {
enum class UploadResult : uint8_t {
  Success = 0, Connection = 1, Unauthorized = 2, Forbidden = 3,
  InvalidToken = 4, InvalidFingerprint = 5, Http = 6, Camera = 7
};
inline UploadResult uploadResult(int status) {
  if (status >= 200 && status < 300) return UploadResult::Success;
  if (status <= 0) return UploadResult::Connection;
  if (status == 401) return UploadResult::Unauthorized;
  if (status == 403) return UploadResult::Forbidden;
  return UploadResult::Http;
}
// The first EEPROM byte stores the length and is part of the reserved size.
inline bool validStoredLength(size_t length, size_t reserved) {
  return length > 0 && length < reserved;
}
inline bool validToken(const char *value, size_t length) {
  if (!value || length != 20) return false;
  for (size_t i = 0; i < length; ++i) {
    const char c = value[i];
    if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
          (c >= '0' && c <= '9'))) return false;
  }
  return true;
}
inline bool validHeaderValue(const char *value, size_t length, size_t reserved) {
  if (!value || !validStoredLength(length, reserved)) return false;
  for (size_t i = 0; i < length; ++i)
    if (static_cast<unsigned char>(value[i]) < 33 ||
        static_cast<unsigned char>(value[i]) > 126) return false;
  return true;
}
inline int hexValue(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  return -1;
}
inline bool urlDecode(const char *value, size_t length, char *out, size_t capacity) {
  size_t n = 0;
  for (size_t i = 0; i < length; ++i) {
    char c = value[i];
    if (c == '%') {
      if (i + 2 >= length) return false;
      const int a = hexValue(value[i + 1]), b = hexValue(value[i + 2]);
      if (a < 0 || b < 0) return false;
      c = static_cast<char>((a << 4) | b);
      i += 2;
    } else if (c == '+') c = ' ';
    if (!c || n + 1 >= capacity) return false;
    out[n++] = c;
  }
  if (n >= capacity) return false;
  out[n] = '\0';
  return true;
}
inline int base64Value(char c) {
  if (c >= 'A' && c <= 'Z') return c - 'A';
  if (c >= 'a' && c <= 'z') return c - 'a' + 26;
  if (c >= '0' && c <= '9') return c - '0' + 52;
  if (c == '-') return 62;
  if (c == '_') return 63;
  return -1;
}
// Public webcam v1 link format observed 2026-10-03. This XOR key is public
// obfuscation, not a credential or encryption key.
inline bool decodeWebcamToken(const char *value, char out[21]) {
  static const char key[] = "webcam-obfuscation-prod-v1-fusekle";
  const size_t length = strlen(value);
  if (length != 30 && length != 31) return false;
  if (memcmp(value, "v1.", 3) || (length == 31 && value[30] != '=')) return false;
  uint32_t buffer = 0;
  unsigned bits = 0;
  size_t count = 0;
  char token[21] = {};
  for (size_t i = 3; i < 30; ++i) {
    const int v = base64Value(value[i]);
    if (v < 0) return false;
    buffer = (buffer << 6) | static_cast<unsigned>(v);
    bits += 6;
    if (bits >= 8) {
      bits -= 8;
      if (count >= 20) return false;
      token[count] = static_cast<char>((buffer >> bits) & 255) ^ key[count % (sizeof(key) - 1)];
      ++count;
    }
  }
  if ((buffer & ((1u << bits) - 1)) || !validToken(token, count)) return false;
  memcpy(out, token, sizeof(token));
  return true;
}
inline bool parameterToken(const char *begin, const char *end, const char *name,
                           bool obfuscated, char out[21]) {
  const size_t nameLength = strlen(name);
  while (begin < end) {
    const char *next = begin;
    while (next < end && *next != '&') ++next;
    if (static_cast<size_t>(next - begin) > nameLength &&
        !memcmp(begin, name, nameLength) && begin[nameLength] == '=') {
      char value[96];
      if (!urlDecode(begin + nameLength + 1, next - begin - nameLength - 1,
                     value, sizeof(value))) return false;
      if (obfuscated) return decodeWebcamToken(value, out);
      if (!validToken(value, strlen(value))) return false;
      memcpy(out, value, 21);
      return true;
    }
    begin = next < end ? next + 1 : end;
  }
  return false;
}
inline bool pairingToken(const char *value, size_t length, char out[21]) {
  if (!value || !out) return false;
  while (length && (*value == ' ' || *value == '\r' || *value == '\n' || *value == '\t')) {
    ++value; --length;
  }
  while (length && (value[length - 1] == ' ' || value[length - 1] == '\r' ||
                    value[length - 1] == '\n' || value[length - 1] == '\t')) --length;
  if (validToken(value, length)) {
    memcpy(out, value, 20); out[20] = '\0'; return true;
  }
  const char *end = value + length;
  const char *fragment = static_cast<const char *>(memchr(value, '#', length));
  if (fragment && parameterToken(fragment + 1, end, "t", true, out)) return true;
  const char *queryEnd = fragment ? fragment : end;
  const char *query = static_cast<const char *>(memchr(value, '?', queryEnd - value));
  return query && parameterToken(query + 1, queryEnd, "token", false, out);
}
} // namespace prusa
#endif
