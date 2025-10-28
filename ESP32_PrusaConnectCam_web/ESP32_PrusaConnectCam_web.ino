/*
   This code is adapted for the M5Stack POEcam Module

   Project: ESP32 PrusaConnect Camera
   Author: Markus Voth
   e-mail: vothmarkus@gmail.com
   Version 1.2 (NTP sync)

   Press and hold the button briefly until the LED flashes slowly to activate QR detection mode.
   Press and hold the button for 5 seconds while it flashes quickly to restart the MCU.
   If set correctly, a photo will be taken every 10s (synchronized to NTP) while the LED flashes for 1s to confirm.

   ERROR flash codes:
   - 2: CONNECTION FAILED
   - 3: NOT AUTHORIZED
   - 4: FORBIDDEN FINGERPRINT
   - 5: INVALID TOKEN DATA
   - 6: INVALID FINGERPRINT DATA
*/

/* includes */
#include <Ticker.h>
#include "Arduino.h"
#include <time.h>              // ⬅️ für NTP sync

#include "server.h"
#include "cfg.h"
#include "var.h"
#include "mcu_cfg.h"
#include "qr.h"

Ticker blinker;
volatile uint8_t blinkCounter = 0;
void blinking()
{
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));
}
void blinkTimes()
{
    digitalWrite(LED_PIN, bool((blinkCounter+1)%2));
    if(blinkCounter > 0)
      --blinkCounter;
    else
    {
        blinker.detach();
        digitalWrite(LED_PIN, 1-LOW);
    }
    return;
}

void GPIO_Init()
{
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, 1-LOW);
    
    pinMode(FLASH_PIN, OUTPUT);
    digitalWrite(FLASH_PIN, LOW);

    pinMode(BUTTON_PIN, INPUT);

    return;
}

void getGPIOreset() //Flip image horrizontally and reboot
{
    /* remember call time */
    uint32_t resetStart = millis();
        
    /* blink quickly */
    blinker.attach(0.1, blinking);

    /* reboot ESP when holding button for 5s */
    while(!digitalRead(BUTTON_PIN))
    {
        if(millis() > resetStart + 5*1000)
        {
            Cfg_ToggleHmirror();
            ESP.restart();
        }
        delay(0);
    }

    blinker.attach(0.25, blinking);
}
 
// Ethernet event handler
static bool eth_connected = false;

void onEvent(arduino_event_id_t event, arduino_event_info_t info)
{
  switch (event) {
    case ARDUINO_EVENT_ETH_START:
      Serial.println("ETH Started");
      
      /* Get MAC Address */
      ETH.macAddress(macAddr);
      char buf[18];
      snprintf(buf, sizeof(buf), "%02X:%02X:%02X:%02X:%02X:%02X",
           macAddr[0], macAddr[1], macAddr[2],
           macAddr[3], macAddr[4], macAddr[5]);
      EthernetMacAddr = buf;
      
      /* Get Ethernet Device Name */
      char suffix[7];
      snprintf(suffix, sizeof(suffix), "%02X%02X%02X", macAddr[3], macAddr[4], macAddr[5]);
      EthernetDeciveName = String("prusaCAM-") + suffix;
      
      // Set Ethernet hostname here
      ETH.setHostname(EthernetDeciveName.c_str());
      Serial.println("ETH MAC: " + EthernetMacAddr);
      Serial.println("ETH Name: " + EthernetDeciveName);
      break;
    case ARDUINO_EVENT_ETH_CONNECTED:
      Serial.println("ETH Connected");
      break;
    case ARDUINO_EVENT_ETH_GOT_IP:
      Serial.println("ETH Got IP: " + String(esp_netif_get_desc(info.got_ip.esp_netif)));
      Serial.println(ETH);
      eth_connected = true;
      break;
    case ARDUINO_EVENT_ETH_LOST_IP:
      Serial.println("ETH Lost IP");
      eth_connected = false;
      break;
    case ARDUINO_EVENT_ETH_DISCONNECTED:
      Serial.println("ETH Disconnected");
      eth_connected = false;
      break;
    case ARDUINO_EVENT_ETH_STOP:
      Serial.println("ETH Stopped");
      eth_connected = false;
      break;
    default:
      break;
  }
}

void ETH_Init()
{
    Network.onEvent(onEvent);
    SPI.begin(ETH_SPI_SCK, ETH_SPI_MISO, ETH_SPI_MOSI, ETH_PHY_CS);
    ETH.begin(ETH_PHY_TYPE, ETH_PHY_ADDR, ETH_PHY_CS, ETH_PHY_IRQ, ETH_PHY_RST, SPI);
}

// --- QR-Code Handling ---
void getQR()
{
    Camera_Reinit(0, true);
    String token = "";
    while(token.isEmpty())
    {
        Camera_CapturePhoto();
        token = qrCodeDetect();
        delay(100);

        getGPIOreset();
    }
    if(!token.isEmpty())
    {
        Cfg_SaveToken(token);
        sToken = token;
    }
    else
    {
        Serial.println("QR timeout: No QR-Code was detected within " + String(QR_TIMEOUT) + "s");
    }
    Camera_Reinit();

    return;
}

// --- NTP Setup ---
#define MY_TZ  "CET-1CEST,M3.5.0,M10.5.0/3"
int lastPhotoSecond = -1;

void NTP_Init()
{
    configTzTime(MY_TZ, "pool.ntp.org", "time.nist.gov");
    struct tm timeinfo;
    while (!getLocalTime(&timeinfo)) {
        Serial.println("Waiting for NTP time...");
        delay(500);
    }
    Serial.println("Time synchronized!");
}

// --- Main setup ---
void setup() {
  /* Serial port for debugging purposes */
  Serial.begin(SERIAL_PORT_SPEER);
  Serial.println("Start MCU!");
  Serial.print("SW Version: ");
  Serial.println(SW_VERSION);

  /* read cfg from EEPROM */
  Cfg_Init();

  /* set GPIOs */
  GPIO_Init();

  /* init camera interface */
  Camera_InitCamera();
  
  /* ethernet init */
  ETH_Init();

  /* NTP init */
  NTP_Init();

  Serial.println("MCU configuration done!");
}

SET_LOOP_TASK_STACK_SIZE(40*1024); //40KB stack for QR-Code lib "quirc"

// --- Main loop ---
void loop()
{
    if(!eth_connected)
    {
        /* ethernet not connected: blink slowly*/
        if(!blinkCounter)
        {
            blinkCounter = 1;
            blinker.attach(1, blinking);
        }
    }
    else
    {
        /* ...connected: stop blinking slowly*/ 
        blinkCounter = 0;
        blinker.detach();
        digitalWrite(LED_PIN, 1-LOW);

        /* time-synchronized photo every 10s */
        struct tm timeinfo;
        if (getLocalTime(&timeinfo)) {
          int sec = timeinfo.tm_sec;
          if (sec % 10 == 0 && sec != lastPhotoSecond) {
            lastPhotoSecond = sec;

            Camera_CapturePhoto();

            /* send photo to backend & get error code*/
            blinkCounter = Server_SendPhotoToPrusaBackend();
            
            /* signal taking photo or error */
            if(blinkCounter)
            {
                blinkCounter = 2*(blinkCounter+1);
                blinker.attach(0.25, blinkTimes);
            }
            else
            {
                blinkCounter = 2;
                blinker.attach(0.5, blinkTimes);
            }
          }
        }
    }
    
    /* press button to enter QR-Mode */ 
    if(!digitalRead(BUTTON_PIN))
    {
        /* blink & check for QR-Codes within 30s */
        blinker.attach(0.25, blinking);
        getQR();
        
        blinker.detach();
        digitalWrite(LED_PIN, 1-LOW);
    }
}

/* EOF */
