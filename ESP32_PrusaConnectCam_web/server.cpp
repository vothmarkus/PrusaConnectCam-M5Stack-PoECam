#include "server.h"

/* send photo to prusa backend server */
uint8_t Server_SendPhotoToPrusaBackend() {
  uint8_t error = 0;

  /* check fingerprint and token length */
  if ((sFingerprint.length() > 0) && (sToken.length() > 0))
  {
    Serial.println("\nConnecting to server...");

    NetworkClientSecure client;
    client.setCACert(rootCA);

    /* connecting to server */
    if (!client.connect(DOMAIN, 443))
    {
        error = 1;
        Serial.println("CONNECTION FAILED! Error" + String(error));
    } 
    else
    {
      /* send data to server */
      Serial.println("Connected to server! Sending image with:");
      Serial.println("fingerprint: " + sFingerprint);
      Serial.println("token: " + sToken);
      
      client.println("PUT " + String(HOST_URL) + " HTTP/1.0");
      client.println("Host: " + String(DOMAIN));
      client.println("User-Agent: ESP32-CAM");
      client.println("Connection: close");

      client.println("Content-Type: image/jpg");
      client.println("fingerprint: " + sFingerprint);
      client.println("token: " + sToken);
      client.print("Content-Length: ");
      client.println(photo.length());
      client.println();

      // photo fragmentation
      esp_task_wdt_reset();
      for (int index = 0; index < photo.length(); index = index + PHOTO_FRAGMENT_SIZE) {
        client.print(photo.substring(index, index + PHOTO_FRAGMENT_SIZE));
        Serial.println(index);
      }
      
      Serial.println("Send done!");
      //esp_task_wdt_reset();
      
      Serial.println("Waiting for response...?!");
      unsigned long ms = millis();
      while (!client.available() && millis() - ms < 1000)
      {
          delay(0);
      }
      Serial.println();
      
      /* check response header */
      ms = millis();
      while (client.connected() && millis() - ms < 500) {
        String line = client.readStringUntil('\n');
        if (line == "\r") {
          Serial.print("Headers received: ");
          Serial.println(line);
          break;
        }
      }

      /* check response data */
      Serial.print("Received data[");
      BackendReceivedData = "";
      while (client.available()) {
        BackendReceivedData += (char) client.read();
      }
      Serial.print(BackendReceivedData.length());
      Serial.print("]: ");
      Serial.println(BackendReceivedData);
      if(BackendReceivedData.indexOf("UNAUTHORIZED") != -1)
        error = 2;
      else if(BackendReceivedData.indexOf("FORBIDDEN_INVALID_FINGERPRINT") != -1)
        error = 3;
        
      client.stop();
    }
  } 
  else
  {
    /* err message */
    Serial.println("ERROR SEND PICTURE TO SERVER! INVALID DATA!");
    Serial.print("Fingerprint: ");
    Serial.println(sFingerprint);
    Serial.print("Token: ");
    Serial.println(sToken);
    if (sToken.length() == 0)
    {
        error = 4;
        Serial.println("INVALID TOKEN DATA! Error " + String(error));
    }
    else if (sFingerprint.length() == 0)
    {
        error = 5;
        Serial.println("INVALID FINGERPRINT DATA! Error " + String(error));
    }
  }
  return error;
}

/* EOF */
