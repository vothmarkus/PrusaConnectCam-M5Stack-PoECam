#include "var.h"

unsigned long lastMillis = 0;

String photo = "";
size_t height = 0;
size_t width = 0;
uint8_t RefreshInterval = 0;
String BackendReceivedData = "";

String sToken = "";
String sFingerprint = "";
String EthernetMacAddr = "";
String EthernetDeciveName = "";
uint8_t macAddr[6] = {0};

struct CameraCfg_struct CameraCfg;

/* EOF */
