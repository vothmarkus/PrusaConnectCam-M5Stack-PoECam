#ifndef _SERVER_H_
#define _SERVER_H_

#include <esp_task_wdt.h>
#include <EEPROM.h>

#include <ETH.h>
#include <SPI.h>
#include <NetworkClientSecure.h>
#include "Certificate.h"

#include "mcu_cfg.h"
#include "var.h"
#include "camera.h"
#include "cfg.h"

uint8_t Server_SendPhotoToPrusaBackend();

#endif

/* EOF */
