#pragma once
#include <time.h>

void Ota_Begin();
void Ota_SetHardwareHealth(bool cameraReady, bool ethernetReady);
void Ota_Loop(bool networkReady, time_t now);
