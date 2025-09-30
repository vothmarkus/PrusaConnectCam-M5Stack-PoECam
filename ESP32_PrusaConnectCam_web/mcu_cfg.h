#ifndef _MCU_CFG_H_
#define _MCU_CFG_H_

#define SW_VERSION         "1.0.0"

#define WDT_TIMEOUT         50              /* wdg timeout second */
#define QR_TIMEOUT          30              /* QR reading timeout second */

/* ---------------- WEB CFG  ----------------*/
#define HOST_URL           "https://webcam.connect.prusa3d.com/c/snapshot" /* Prusa Connect server URL for photo */
#define DOMAIN             "webcam.connect.prusa3d.com"                    /* prusa connect server */
#define PHOTO_FRAGMENT_SIZE 9000            /* photo fragmentation size */

/* ---------------- GPIO CFG  ----------------*/
#define LED_PIN 0
#define FLASH_PIN 25
#define BUTTON_PIN 37
#define SERIAL_PORT_SPEER   115200          /* baud rate */

/* ---------------- ETH CFG  ------------------*/
#define USE_TWO_ETH_PORTS 0
 
#define ETH_PHY_TYPE        ETH_PHY_W5500
#define ETH_PHY_ADDR        1
#define ETH_PHY_CS          4
#define ETH_PHY_IRQ         -1
#define ETH_PHY_RST         -1
 
// SPI pins
#define ETH_SPI_SCK         23
#define ETH_SPI_MISO        38
#define ETH_SPI_MOSI        13

/* ---------------- CFG FLAGS  ----------------*/
#define CFG_FIRST_MCU_START_ACK               0xFF    ///< flag first MCU start ACK -> yes, it's first mcu start
#define CFG_FIRST_MCU_START_NAK               0x0B    ///< flag first MCU start NAK -> no, it's not first MCU start

/* ---------------- EEPROM CFG ----------------*/

#define EEPROM_ADDR_REFRESH_INTERVAL_START    0     /* whre is stored first byte from refresh data */
#define EEPROM_ADDR_REFRESH_INTERVAL_LENGTH   1     /* how long is the refresh data variable stored in the eeprom [bytes] */

#define EEPROM_ADDR_FINGERPRINT_START         (EEPROM_ADDR_REFRESH_INTERVAL_START + EEPROM_ADDR_REFRESH_INTERVAL_LENGTH)    /* where is stored first byte from fingerprint */
#define EEPROM_ADDR_FINGERPRINT_LENGTH        80                                                                            /* how long is fingerprint [bytes] */

#define EEPROM_ADDR_TOKEN_START               (EEPROM_ADDR_FINGERPRINT_START + EEPROM_ADDR_FINGERPRINT_LENGTH)              /* where is stored first byte from token */
#define EEPROM_ADDR_TOKEN_LENGTH              40                                                                            /* how long is token [bytes] */

#define EEPROM_ADDR_FRAMESIZE_START           (EEPROM_ADDR_TOKEN_START + EEPROM_ADDR_TOKEN_LENGTH)
#define EEPROM_ADDR_FRAMESIZE_LENGTH          1

#define EEPROM_ADDR_BRIGHTNESS_START          (EEPROM_ADDR_FRAMESIZE_START + EEPROM_ADDR_FRAMESIZE_LENGTH)
#define EEPROM_ADDR_BRIGHTNESS_LENGTH         1

#define EEPROM_ADDR_CONTRAST_START            (EEPROM_ADDR_BRIGHTNESS_START + EEPROM_ADDR_BRIGHTNESS_LENGTH)
#define EEPROM_ADDR_CONTRAST_LENGTH           1

#define EEPROM_ADDR_SATURATION_START          (EEPROM_ADDR_CONTRAST_START + EEPROM_ADDR_CONTRAST_LENGTH)
#define EEPROM_ADDR_SATURATION_LENGTH         1

#define EEPROM_ADDR_HMIRROR_START             (EEPROM_ADDR_SATURATION_START + EEPROM_ADDR_SATURATION_LENGTH)
#define EEPROM_ADDR_HMIRROR_LENGTH            1

#define EEPROM_ADDR_VFLIP_START               (EEPROM_ADDR_HMIRROR_START + EEPROM_ADDR_HMIRROR_LENGTH)
#define EEPROM_ADDR_VFLIP_LENGTH              1

#define EEPROM_ADDR_LENSC_START               (EEPROM_ADDR_VFLIP_START + EEPROM_ADDR_VFLIP_LENGTH)
#define EEPROM_ADDR_LENSC_LENGTH              1

#define EEPROM_ADDR_EXPOSURE_CTRL_START       (EEPROM_ADDR_LENSC_START + EEPROM_ADDR_LENSC_LENGTH)
#define EEPROM_ADDR_EXPOSURE_CTRL_LENGTH      1

#define EEPROM_ADDR_PHOTO_QUALITY_START       (EEPROM_ADDR_EXPOSURE_CTRL_START + EEPROM_ADDR_EXPOSURE_CTRL_LENGTH)
#define EEPROM_ADDR_PHOTO_QUALITY_LENGTH      1

#define EEPROM_ADDR_FIRST_MCU_START_FLAG_START    (EEPROM_ADDR_PHOTO_QUALITY_START + EEPROM_ADDR_PHOTO_QUALITY_LENGTH)  
#define EEPROM_ADDR_FIRST_MCU_START_FLAG_LENGTH   1                                                                                       

#define EEPROM_ADDR_CAMERA_FLASH_FUNCTION         (EEPROM_ADDR_FIRST_MCU_START_FLAG_START + EEPROM_ADDR_FIRST_MCU_START_FLAG_LENGTH)
#define EEPROM_ADDR_CAMERA_FLASH_FUNCTION_LENGTH  1

#define EEPROM_ADDR_CAMERA_FLASH_DURATION         (EEPROM_ADDR_CAMERA_FLASH_FUNCTION + EEPROM_ADDR_CAMERA_FLASH_FUNCTION_LENGTH)
#define EEPROM_ADDR_CAMERA_FLASH_DURATION_LENGTH  2

#define EEPROM_SIZE                           (EEPROM_ADDR_REFRESH_INTERVAL_LENGTH + EEPROM_ADDR_FINGERPRINT_LENGTH + EEPROM_ADDR_TOKEN_LENGTH + EEPROM_ADDR_FRAMESIZE_LENGTH + EEPROM_ADDR_BRIGHTNESS_LENGTH + EEPROM_ADDR_CONTRAST_LENGTH + EEPROM_ADDR_SATURATION_LENGTH + EEPROM_ADDR_HMIRROR_LENGTH + EEPROM_ADDR_VFLIP_LENGTH + EEPROM_ADDR_LENSC_LENGTH + EEPROM_ADDR_EXPOSURE_CTRL_LENGTH + EEPROM_ADDR_PHOTO_QUALITY_LENGTH + EEPROM_ADDR_FIRST_MCU_START_FLAG_LENGTH + EEPROM_ADDR_CAMERA_FLASH_FUNCTION_LENGTH + EEPROM_ADDR_CAMERA_FLASH_DURATION_LENGTH) /* how many bits do we need for eeprom memory */

#endif

/* EOF */
