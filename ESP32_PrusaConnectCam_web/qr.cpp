#include "qr.h"
#include "protocol.h"
String qrCodeDetect() {
  if (!photoFrame || photoFrame->format != PIXFORMAT_GRAYSCALE ||
      !photoFrame->buf || !photoFrame->width || !photoFrame->height ||
      photoFrame->width > 1600 || photoFrame->height > 1200 ||
      photoFrame->len != photoFrame->width * photoFrame->height) return "";
  struct quirc *qr = quirc_new();
  if (!qr) {
    Serial.println("QR allocation failed");
    return "";
  }
  if (quirc_resize(qr, photoFrame->width, photoFrame->height) < 0) {
    Serial.println("QR image allocation failed");
    quirc_destroy(qr);
    return "";
  }
  uint8_t *image = quirc_begin(qr, nullptr, nullptr);
  memcpy(image, photoFrame->buf, photoFrame->len);
  quirc_end(qr);
  String token;
  for (int i = 0; i < quirc_count(qr); ++i) {
    struct quirc_code code;
    struct quirc_data data;
    quirc_extract(qr, i, &code);
    quirc_decode_error_t error = quirc_decode(&code, &data);
    if (error == QUIRC_ERROR_DATA_ECC) {
      quirc_flip(&code);
      error = quirc_decode(&code, &data);
    }
    if (error) {
      Serial.printf("QR decode failed: %s\n", quirc_strerror(error));
      continue;
    }
    char parsed[21];
    if (data.payload_len > 0 && static_cast<size_t>(data.payload_len) < sizeof(data.payload) &&
        prusa::pairingToken(reinterpret_cast<const char *>(data.payload), data.payload_len, parsed)) {
      token = parsed;
      break;
    }
  }
  quirc_destroy(qr);
  return token;
}
