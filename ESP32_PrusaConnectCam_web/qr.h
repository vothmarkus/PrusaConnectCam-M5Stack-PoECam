#ifndef _QR_H_
#define _QR_H_

#include "Arduino.h"
#include "camera.h"

#include "quirc.h"

String qrCodeDetect();
String extractToken(String);

#endif

/* EOF */
