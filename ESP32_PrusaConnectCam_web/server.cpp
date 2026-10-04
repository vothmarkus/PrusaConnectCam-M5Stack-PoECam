#include "server.h"

uint8_t Server_SendPhotoToPrusaBackend() {
  using prusa::UploadResult;
  if (!prusa::validToken(sToken.c_str(), sToken.length()))
    return static_cast<uint8_t>(UploadResult::InvalidToken);
  if (!prusa::validHeaderValue(sFingerprint.c_str(), sFingerprint.length(), EEPROM_ADDR_FINGERPRINT_LENGTH))
    return static_cast<uint8_t>(UploadResult::InvalidFingerprint);
  if (!photoFrame || !photoFrame->buf || !photoFrame->len || photoFrame->format != PIXFORMAT_JPEG)
    return static_cast<uint8_t>(UploadResult::Camera);

  NetworkClientSecure client;
  client.setCACert(rootCA);
  client.setHandshakeTimeout(TLS_HANDSHAKE_TIMEOUT_S);
  HTTPClient http;
  http.setConnectTimeout(HTTP_CONNECT_TIMEOUT_MS);
  http.setTimeout(HTTP_READ_TIMEOUT_MS);
  http.setReuse(false);
  // Never forward camera credentials to a redirect destination.
  http.setFollowRedirects(HTTPC_DISABLE_FOLLOW_REDIRECTS);
  if (!http.begin(client, HOST_URL)) {
    Serial.println("Unable to initialize HTTPS upload");
    return static_cast<uint8_t>(UploadResult::Connection);
  }
  http.setUserAgent("M5PoECAM-PrusaConnect/" SW_VERSION);
  http.addHeader("Content-Type", "image/jpeg");
  http.addHeader("Token", sToken);
  http.addHeader("Fingerprint", sFingerprint);
  Serial.printf("Uploading %u bytes to %s\n", static_cast<unsigned>(photoFrame->len), DOMAIN);
  const int status = http.sendRequest("PUT", photoFrame->buf, photoFrame->len);
  if (status > 0) {
    Serial.printf("Snapshot HTTP status: %d\n", status);
    if (status >= 300 && status < 400)
      Serial.println("Redirect rejected: check the configured API endpoint");
    else if (status == 401 || status == 403)
      Serial.println("Camera access denied: check pairing/token and stored fingerprint");
    else if (status == 429)
      Serial.println("Upload rate limited by server");
  } else {
    Serial.printf("HTTPS upload failed: %s (%d)\n", HTTPClient::errorToString(status).c_str(), status);
    char message[160] = {};
    const int tlsError = client.lastError(message, sizeof(message));
    if (tlsError) Serial.printf("TLS error %d: %s\n", tlsError, message);
  }
  // Only 2xx is success; no response body or credentials are logged.
  http.end();
  client.stop();
  return static_cast<uint8_t>(prusa::uploadResult(status));
}
