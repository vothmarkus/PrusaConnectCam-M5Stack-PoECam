#include "var.h"

camera_fb_t *photoFrame = nullptr;
uint8_t RefreshInterval = 0;

String sToken = "";
String sFingerprint = "";
String EthernetMacAddr = "";
String EthernetDeciveName = "";
uint8_t macAddr[6] = {0};

struct CameraCfg_struct CameraCfg;

/* EOF */
