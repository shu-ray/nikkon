#include "esp_camera.h"
#include "Arduino.h"
#include "FS.h"               // SD Card ESP32
#include "SD_MMC.h"           // SD Card ESP32
#include "soc/soc.h"          // Disable brownour problems
#include "soc/rtc_cntl_reg.h" // Disable brownour problems
#include "driver/rtc_io.h"
#include <EEPROM.h> // read and write from flash memory

// define the number of bytes you want to access
#define EEPROM_SIZE 1

// Pin definition for CAMERA_MODEL_AI_THINKER
#define PWDN_GPIO_NUM 32
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM 0
#define SIOD_GPIO_NUM 26
#define SIOC_GPIO_NUM 27

#define Y9_GPIO_NUM 35
#define Y8_GPIO_NUM 34
#define Y7_GPIO_NUM 39
#define Y6_GPIO_NUM 36
#define Y5_GPIO_NUM 21
#define Y4_GPIO_NUM 19
#define Y3_GPIO_NUM 18
#define Y2_GPIO_NUM 5
#define VSYNC_GPIO_NUM 25
#define HREF_GPIO_NUM 23
#define PCLK_GPIO_NUM 22

void configESPCamera()
{
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
  config.frame_size = FRAMESIZE_UXGA;   // FRAMESIZE_ + QVGA|CIF|VGA|SVGA|XGA|SXGA|UXGA
  config.jpeg_quality = 10;             // 10-63 lower number means higher quality
  config.fb_count = 2;

  // Initialize the Camera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK)
  {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  // Camera quality adjustments
  sensor_t *s = esp_camera_sensor_get();

  s->set_framesize(s, FRAMESIZE_UXGA);     // FRAMESIZE_[QQVGA|HQVGA|QVGA|CIF|VGA|SVGA|XGA|SXGA|UXGA|QXGA(ov3660)]);
  s->set_quality(s, 10);                   // 10 to 63
  s->set_brightness(s, 0);                 // -2 to 2
  s->set_contrast(s, 0);                   // -2 to 2
  s->set_saturation(s, 0);                 // -2 to 2
  s->set_special_effect(s, 0);             // 0 to 6 (0 - No Effect, 1 - Negative, 2 - Grayscale, 3 - Red Tint, 4 - Green Tint, 5 - Blue Tint, 6 - Sepia)
  s->set_whitebal(s, 1);                   // aka 'awb' in the UI; 0 = disable , 1 = enable
  s->set_awb_gain(s, 1);                   // 0 = disable , 1 = enable
  s->set_wb_mode(s, 0);                    // 0 to 4 - if awb_gain enabled (0 - Auto, 1 - Sunny, 2 - Cloudy, 3 - Office, 4 - Home)
  s->set_exposure_ctrl(s, 1);              // 0 = disable , 1 = enable
  s->set_aec2(s, 0);                       // 0 = disable , 1 = enable
  s->set_ae_level(s, 0);                   // -2 to 2
  s->set_aec_value(s, 300);                // 0 to 1200
  s->set_gain_ctrl(s, 1);                  // 0 = disable , 1 = enable
  s->set_agc_gain(s, 0);                   // 0 to 30
  s->set_gainceiling(s, (gainceiling_t)0); // 0 to 6
  s->set_bpc(s, 0);                        // 0 = disable , 1 = enable
  s->set_wpc(s, 1);                        // 0 = disable , 1 = enable
  s->set_raw_gma(s, 1);                    // 0 = disable , 1 = enable
  s->set_lenc(s, 1);                       // 0 = disable , 1 = enable
  s->set_hmirror(s, 0);                    // 0 = disable , 1 = enable
  s->set_vflip(s, 1);                      // 0 = disable , 1 = enable
  s->set_dcw(s, 1);                        // 0 = disable , 1 = enable
  s->set_colorbar(s, 0);                   // 0 = disable , 1 = enable
}

void initMicroSDCard()
{
  // Start the MicroSD card

  Serial.println("Mounting MicroSD Card");
  if (!SD_MMC.begin("/sdcard", true))
  {
    Serial.println("MicroSD Card Mount Failed");
    return;
  }
  uint8_t cardType = SD_MMC.cardType();
  if (cardType == CARD_NONE)
  {
    Serial.println("No MicroSD Card found");
    return;
  }
}

class TouchSensor {
  public:
    int threshold;
    byte PIN;
    bool touchFlag = false;
    bool checkTouchFlag = false;  // for comparing with touchFlag

    TouchSensor(byte GPIO_PIN,int senseThreshold){
      threshold = senseThreshold;
      PIN = GPIO_PIN;
    }

    bool pressing(){
      if (touchRead(PIN) < threshold){
        touchFlag = true;
        return true;
        }
        touchFlag = false;
        return false;
    }
    bool just_pressed(){
      checkTouchFlag = touchFlag;
      if (checkTouchFlag != pressing() && checkTouchFlag == false){  // pressing() is called first thus touchFlag is changed to true in pressing()
        return true;
      }
      return false;
    }
    bool just_released(){
      checkTouchFlag = touchFlag;
      if (checkTouchFlag != pressing() && checkTouchFlag == true){
        return true;
      }
      return false;
    }
};

class EEPROMCount {
  public:
    int readAddr;
    int count;
    EEPROMCount(){
    // Store bytes address in the first eeprom byte address 0x0
      readAddr = EEPROM.read(0);
    }

    int retrieveCount()
    {
      // if  the address is full  or  the address to be read is 0 (only if somehow eeprom is erased so it will read itself)
      if (EEPROM.read(readAddr) == 256 || readAddr == 0){
        readAddr++;
        EEPROM.write(0,readAddr);
        EEPROM.commit();
      }
      for (int i = 1; i <= readAddr; i++){
        count += EEPROM.read(i);
      }
      return count;
  }
    void setNextCount(){
      count++;
      EEPROM.write(readAddr,count);
  }
};

void camCalibrate(){
  Serial.println("Cabbing...");
  byte count = 0;
  unsigned long now = millis();
  while (now < 6000){
    now = millis();
    if (now % 100 == 0){
      count++;
      camera_fb_t *fb = esp_camera_fb_get();
      delayMicroseconds(500);
      if (!fb){
        Serial.println("Cab failed");
        return;
      }

      esp_camera_fb_return(fb);      
      Serial.print(count);
    }
    delayMicroseconds(500);
  }
  Serial.println("\nCab done");
}

void setup()
{
  // Disable brownout detector
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

  // Start Serial Monitor
  Serial.begin(115200);

  // Initialize the camera
  Serial.print("Initializing the camera module...");
  configESPCamera();
  Serial.println("Camera OK!");

  // Initialize the MicroSD
  Serial.print("Initializing the MicroSD card module... ");
  initMicroSDCard();

  // initialize EEPROM with predefined size
  EEPROM.begin(8);



//  delay(6000);
  // Serial.println("Entering sleep mode");
  // delay(1000);

  // // Enter deep sleep mode
  // esp_deep_sleep_start();

  camCalibrate();

  //while (true){;}
}

touch_pad_t touchPin;
TouchSensor camButton(12, 30);
EEPROMCount pictureCount;

void loop()
{
  if (camButton.just_pressed()){
    takeNewPhoto();
  }
}

void takeNewPhoto(){

  // Path where new picture will be saved in SD Card
  String path = "/image" + String(pictureCount.retrieveCount()) + ".jpg";
  Serial.printf("Picture file name: %s\n", path.c_str());

  // Save picture to microSD card
  fs::FS &fs = SD_MMC;
  File file = fs.open(path.c_str(), FILE_WRITE);
  if (!file){
    Serial.println("Failed to open file in write mode");
    return;
  }

  // Setup frame buffer
  camera_fb_t *fb = esp_camera_fb_get();

  if (!fb){
    Serial.println("Camera capture failed");
    return;
  }

  file.write(fb->buf, fb->len); // payload (image), payload length
  Serial.printf("Saved file to path: %s\n", path.c_str());
  // Close the file
  file.close();

  // Return the frame buffer back to the driver for reuse
  esp_camera_fb_return(fb);

  // Update EEPROM picture number counter
  pictureCount.setNextCount();
}

