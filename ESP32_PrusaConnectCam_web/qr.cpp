#include "qr.h"

String qrCodeDetect()
{   
    String token = "";
        
    struct quirc *qr = NULL;
  
    qr = quirc_new();
    if (qr == NULL)
    {
        Serial.println("Error to alocate MEMORY");
        return "";
    }

    if (quirc_resize(qr, width, height) < 0)
    {
        Serial.println("Error to alocate video MEMORY");
        quirc_destroy(qr);
        return "";
    }

    Serial.println("quirc_begin");
    uint8_t *image = NULL;
    image = quirc_begin(qr, NULL, NULL);
    memcpy(image, photo.c_str(), photo.length());
    quirc_end(qr);
    
    int count = quirc_count(qr);
    Serial.println("Count: " + String(count));

    for (int i = 0; i < count; i++)
    {

      struct quirc_code code;
      struct quirc_data data;
      quirc_extract(qr, i, &code);
      
      if(!quirc_decode(&code, &data))
      {
          Serial.print("Payload: ");
          Serial.println((const char *)data.payload);
          token = extractToken(String((const char *)data.payload));
          Serial.println("Token: " + token);
      }
    }

    image = NULL;  
    quirc_destroy(qr);
    return token;
}

String extractToken(String url)
{
  int tokenIndex = url.indexOf("token=");
  if (tokenIndex != -1)
  {
    tokenIndex += 6; // Bewegen zum Beginn des Tokens
    String token = url.substring(tokenIndex);
    return token;
  }
  return String("");
}

/* EOF */
