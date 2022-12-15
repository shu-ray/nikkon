// Camera libraries
#include "esp_camera.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "driver/rtc_io.h"

// MicroSD Libraries
#include "FS.h"
#include "SD_MMC.h"

// Counter for picture number
unsigned int pictureCount = 0;
unsigned int nextPictureCount = 0;
const String PICTURE_COUNT_PATH = "/PICTURE_COUNT_PATH";

// Pin definitions for CAMERA_MODEL_AI_THINKER
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

void configESPCamera() {
  // Configure Camera parameters

  // Object to store the camera configuration parameters
  camera_config_t config;

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
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG; // Choices are YUV422, GRAYSCALE, RGB565, JPEG
  config.frame_size = FRAMESIZE_UXGA; // FRAMESIZE_ + QVGA|CIF|VGA|SVGA|XGA|SXGA|UXGA
  config.jpeg_quality = 10; //10-63 lower number means higher quality
  config.fb_count = 2;

  // Initialize the Camera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  // Camera quality adjustments
  sensor_t * s = esp_camera_sensor_get();

  // BRIGHTNESS (-2 to 2)
  s->set_brightness(s, 0);
  // CONTRAST (-2 to 2)
  s->set_contrast(s, 0);
  // SATURATION (-2 to 2)
  s->set_saturation(s, 0);
  // SPECIAL EFFECTS (0 - No Effect, 1 - Negative, 2 - Grayscale, 3 - Red Tint, 4 - Green Tint, 5 - Blue Tint, 6 - Sepia)
  s->set_special_effect(s, 0);
  // WHITE BALANCE (0 = Disable , 1 = Enable)
  s->set_whitebal(s, 1);
  // AWB GAIN (0 = Disable , 1 = Enable)
  s->set_awb_gain(s, 1);
  // WB MODES (0 - Auto, 1 - Sunny, 2 - Cloudy, 3 - Office, 4 - Home)
  s->set_wb_mode(s, 0);
  // EXPOSURE CONTROLS (0 = Disable , 1 = Enable)
  s->set_exposure_ctrl(s, 1);
  // AEC2 (0 = Disable , 1 = Enable)
  s->set_aec2(s, 0);
  // AE LEVELS (-2 to 2)
  s->set_ae_level(s, 0);
  // AEC VALUES (0 to 1200)
  s->set_aec_value(s, 300);
  // GAIN CONTROLS (0 = Disable , 1 = Enable)
  s->set_gain_ctrl(s, 1);
  // AGC GAIN (0 to 30)
  s->set_agc_gain(s, 0);
  // GAIN CEILING (0 to 6)
  s->set_gainceiling(s, (gainceiling_t)0);
  // BPC (0 = Disable , 1 = Enable)
  s->set_bpc(s, 0);
  // WPC (0 = Disable , 1 = Enable)
  s->set_wpc(s, 1);
  // RAW GMA (0 = Disable , 1 = Enable)
  s->set_raw_gma(s, 1);
  // LENC (0 = Disable , 1 = Enable)
  s->set_lenc(s, 1);
  // HORIZ MIRROR (0 = Disable , 1 = Enable)
  s->set_hmirror(s, 0);
  // VERT FLIP (0 = Disable , 1 = Enable)
  s->set_vflip(s, 0);
  // DCW (0 = Disable , 1 = Enable)
  s->set_dcw(s, 1);
  // COLOR BAR PATTERN (0 = Disable , 1 = Enable)
  s->set_colorbar(s, 0);

}

class TouchSensor {
  public:
    int threshold = 0;
    byte PIN = 0;
    bool touchingFlag;

    TouchSensor(int senseThreshold, byte GPIO_PIN){
      threshold = senseThreshold;
      PIN = GPIO_PIN;
      touchingFlag = touchRead(PIN);
    }

    bool pressing(){
      if (touchRead(PIN) > threshold){
        touchingFlag = true;
        return true;
        }
        touchingFlag = false;
        return false;
    }
    bool just_pressed(){
      if (touchingFlag != pressing()){
        if (touchingFlag = false){
          touchingFlag = true;
          return true;
        }
      }
      touchingFlag = false;
      return false;
    }
    bool just_released(){
      if (touchingFlag != pressing()){
        if (touchingFlag = true){
          touchingFlag = true;
          return true;
        }
      }
      touchingFlag = false;
      return false;
    }
};

void initMicroSDCard() {
  Serial.println("Mounting MicroSD Card");
  if (!SD_MMC.begin()) {
    Serial.println("MicroSD Card Mount Failed");
    return;
  }
  uint8_t cardType = SD_MMC.cardType();
  if (cardType == CARD_NONE) {
    Serial.println("No MicroSD Card found");
    return;
  }
}

void takeNewPhoto() {
  // Set the next picture pathname
  String path = "/esp32cam/images/image" + String(nextPictureCount) + ".jpg";
  //Serial.printf("Picture file name: %s\n", path.c_str());

  // Get a frame buffer (a picture) and save the buffer to sd card
  camera_fb_t  * fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed");
    return;
  }

  fs::FS &fs = SD_MMC;
  File file = fs.open(path.c_str(), FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open file in write mode");
  }
  else {
    file.write(fb->buf, fb->len); // payload (image), payload length
    Serial.printf("Saved file to path: %s\n", path.c_str());
  }
  file.close();
  esp_camera_fb_return(fb);

  // Save picture index
  File file = fs.open("/pictureIndex", FILE_WRITE);
  file.write(nextPictureCount);
  file.close();

}

// Threshold value to-change
  // T6 is on GPIO 14 @ pin 7

TouchSensor camButton(50, 14);
unsigned long stopWatch = 0;

void callback(){}

void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
  Serial.begin(115200);

  Serial.print("Initializing the camera module...");
  configESPCamera();
  Serial.println("Camera OK!");

  Serial.print("Initializing the MicroSD card module... ");
  initMicroSDCard();

  // Read last picture index to set for new picture pathname
  fs::FS &fs = SD_MMC;
  File file = fs.open(PICTURE_COUNT_PATH, FILE_READ);
  nextPictureCount = file.read() + 1;
  file.close();

  touchAttachInterrupt(T6, callback, camButton.threshold);
  esp_sleep_enable_touchpad_wakeup();

  stopWatch = millis();
  while (millis() - stopWatch < 500){
    if (camButton.just_released()){
      takeNewPhoto();
      delay(100);
      esp_deep_sleep_start();
    }
  }
  // startVideoCapture();
  if (camButton.just_released()){
    // stopVideoCapture()
      delay(100);
      esp_deep_sleep_start();
  }

}

  // // Turns off the ESP32-CAM white on-board LED (flash) connected to GPIO 4
  // pinMode(4, OUTPUT);
  // digitalWrite(4, LOW);
  // rtc_gpio_hold_en(GPIO_NUM_4);

void loop() {                       // Will never run
  if (camButton.pressing()){
    if (!stopWatchStarted){
      stopWatch = millis();
      stopWatchStarted = true;
    }
    if (millis() - stopWatch < 500){
      if (camButton.just_released()){
        takeNewPhoto();
      }
    }
    else {
      // startVideoCapture()
      if (camButton.just_released()){
        // stopVideoCapture()
      }
    }
  }
  else {
    stopWatchStarted = false;
  }
}