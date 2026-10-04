#include "camera.h"

static bool cameraInitialized = false;
void Camera_ReleasePhoto() {
  if (photoFrame) esp_camera_fb_return(photoFrame);
  photoFrame = nullptr;
}
bool Camera_CapturePhoto() {
  Camera_ReleasePhoto();
  if (!cameraInitialized && !Camera_InitCamera()) return false;
  if (CameraCfg.CameraFlashStatus) {
    digitalWrite(FLASH_PIN, HIGH);
    delay(CameraCfg.CameraFlashDuration);
  }
  // Drop the queued frame so the next capture reflects the current scene.
  camera_fb_t *stale = esp_camera_fb_get();
  if (stale) esp_camera_fb_return(stale);
  photoFrame = esp_camera_fb_get();
  digitalWrite(FLASH_PIN, LOW);
  if (!photoFrame || !photoFrame->buf || !photoFrame->len ||
      !photoFrame->width || !photoFrame->height) {
    Camera_ReleasePhoto();
    Serial.println("Camera capture failed");
    return false;
  }
  return true;
}

/* Init camera module */
bool Camera_InitCamera(uint8_t FrameSize, bool GREYSCALE)
{
  Serial.println("Init camera CFG");
  /* Turn-off the 'brownout detector' */
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

  camera_config_t config = {};
  /* OV2640 camera module pinout and cfg*/
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = GREYSCALE ? PIXFORMAT_GRAYSCALE : PIXFORMAT_JPEG; /* YUV422,GRAYSCALE,RGB565,JPEG */

  /* OV2640
    FRAMESIZE_QVGA (320 x 240)
    FRAMESIZE_CIF (352 x 288)
    FRAMESIZE_VGA (640 x 480)
    FRAMESIZE_SVGA (800 x 600)
    FRAMESIZE_XGA (1024 x 768)
    FRAMESIZE_SXGA (1280 x 1024)
    FRAMESIZE_UXGA (1600 x 1200)
  */
  config.frame_size = Cfg_TransformFrameSizeDataType(FrameSize);  /* FRAMESIZE_ + QVGA|CIF|VGA|SVGA|XGA|SXGA|UXGA */
  config.jpeg_quality = CameraCfg.PhotoQuality;                             /*10-63 lower number means higher quality */
  config.fb_count = 1;                                                      /* picture frame buffer alocation */

  config.fb_location = psramFound() ? CAMERA_FB_IN_PSRAM : CAMERA_FB_IN_DRAM;
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  if (!psramFound()) config.frame_size = FRAMESIZE_QVGA;

  /* Camera init */
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed. Error 0x%x", err);
    cameraInitialized = false;
    return false;
  }

  cameraInitialized = true;
  //Write to Sensor
  Camera_SetCameraCfg();

  return true;
}

void Camera_SetCameraCfg() {
  Serial.println("Set camera CFG");

  /* sensor configuration */
  sensor_t * sensor = esp_camera_sensor_get();
  if (!sensor) return;
  sensor->set_brightness(sensor, CameraCfg.brightness);       // -2 to 2
  sensor->set_contrast(sensor, CameraCfg.contrast);           // -2 to 2
  sensor->set_saturation(sensor, CameraCfg.saturation);       // -2 to 2
  sensor->set_special_effect(sensor, 0);                      // 0 to 6 (0 - No Effect, 1 - Negative, 2 - Grayscale, 3 - Red Tint, 4 - Green Tint, 5 - Blue Tint, 6 - Sepia)
  sensor->set_whitebal(sensor, 1);                            // 0 = disable , 1 = enable
  sensor->set_awb_gain(sensor, 1);                            // 0 = disable , 1 = enable
  sensor->set_wb_mode(sensor, 0);                             // 0 to 4 - if awb_gain enabled (0 - Auto, 1 - Sunny, 2 - Cloudy, 3 - Office, 4 - Home)
  sensor->set_exposure_ctrl(sensor, CameraCfg.exposure_ctrl); // 0 = disable , 1 = enable
  sensor->set_aec2(sensor, 0);                                // 0 = disable , 1 = enable
  sensor->set_ae_level(sensor, 0);                            // -2 to 2
  sensor->set_aec_value(sensor, 300);                         // 0 to 1200
  sensor->set_gain_ctrl(sensor, 1);                           // 0 = disable , 1 = enable
  sensor->set_agc_gain(sensor, 0);                            // 0 to 30
  sensor->set_gainceiling(sensor, (gainceiling_t)0);          // 0 to 6
  sensor->set_bpc(sensor, 0);                                 // 0 = disable , 1 = enable
  sensor->set_wpc(sensor, 1);                                 // 0 = disable , 1 = enable
  sensor->set_raw_gma(sensor, 1);                             // 0 = disable , 1 = enable
  sensor->set_lenc(sensor, CameraCfg.lensc);                  // 0 = disable , 1 = enable
  sensor->set_hmirror(sensor, CameraCfg.hmirror);             // 0 = disable , 1 = enable
  sensor->set_vflip(sensor, CameraCfg.vflip);                 // 0 = disable , 1 = enable
  sensor->set_dcw(sensor, 1);                                 // 0 = disable , 1 = enable
  sensor->set_colorbar(sensor, 0);                            // 0 = disable , 1 = enable
}

bool Camera_Reinit(uint8_t FrameSize, bool GREYSCALE) {
  Camera_ReleasePhoto();
  if (cameraInitialized) esp_camera_deinit();
  cameraInitialized = false;
  return Camera_InitCamera(FrameSize, GREYSCALE);
}


/* EOF */
