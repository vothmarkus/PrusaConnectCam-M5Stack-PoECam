#ifndef _VARIABLE_H_
#define _VARIABLE_H_

#include "Arduino.h"
#include "mcu_cfg.h"
#include "esp_camera.h"


extern camera_fb_t *photoFrame;       /* owned until Camera_ReleasePhoto() */
extern uint8_t RefreshInterval;

extern String sToken;                 /* token for authentification to prusa backend */
extern String sFingerprint;           /* fingerprint for autentification to prusa backend */
extern String EthernetMacAddr;        /* Ethernet MAC address */
extern String EthernetDeciveName;     /* Ethernet Decive Name */
extern uint8_t macAddr[6];

struct CameraCfg_struct {
  uint8_t PhotoQuality;
  uint8_t FrameSize;
  int8_t brightness;
  int8_t contrast;
  int8_t saturation;

  bool hmirror;
  bool vflip;
  bool lensc;
  bool exposure_ctrl;

  bool CameraFlashStatus;
  uint16_t CameraFlashDuration;
};

extern struct CameraCfg_struct CameraCfg;

#endif

/* EOF */
