#include "esp_camera.h"
#include "Arduino.h"
#include "FS.h"               // SD Card ESP32
#include "SD_MMC.h"           // SD Card ESP32
#include "soc/soc.h"          // Disable brownour problems
#include "soc/rtc_cntl_reg.h" // Disable brownour problems
#include "driver/rtc_io.h"
#include <OneButton.h>

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

#define BUTTON_PIN 12
char * countPath = "/camSettings/picIndex.txt";

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
  config.frame_size = FRAMESIZE_HD;   // FRAMESIZE_ + QVGA|CIF|VGA|SVGA|XGA|SXGA|UXGA
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

  s->set_framesize(s, FRAMESIZE_HD);     // FRAMESIZE_[QQVGA|HQVGA|QVGA|CIF|VGA|SVGA|XGA|SXGA|UXGA|QXGA(ov3660)]);
  s->set_quality(s, 8);                   // 10 to 63
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
  s->set_vflip(s, 0);                      // 0 = disable , 1 = enable
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

void printFile(const char * path, int message){
  fs::FS &fs = SD_MMC;
  File file = fs.open(path, FILE_WRITE);
  if(!file){
    Serial.println("Failed to open file for writing");
    return;
  }
  file.print(message);
  file.close();
}

String readFile(const char * path){
  fs::FS &fs = SD_MMC;
  File file = fs.open(path);
  if(!file){
    Serial.println("Failed to open file for reading");
  }

  String temp = "";
  while(file.available()){
    temp += (char)file.read();
  }
  file.close();
  return temp;;
}

class FileCount{
  public:
    char * countFilePat = "";
    String countVal = "";
    FileCount(char * countFilePath){
      countFilePat = countFilePath;
    }

    String getCount(){
      countVal = readFile(countFilePat);
      return countVal;
    }

    void setNextCount(){
      printFile(countFilePat,countVal.toInt() + 1);
    }
};

FileCount Count(countPath);


// In honor to jameszah's esp32-cam-video-recorder-junior-60
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Make the avi functions
//
//   start_avi() - open the file and write headers
//   another_pic_avi() - write one more frame of movie
//   end_avi() - write the final parameters and close the file
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// start_avi - open the files and write in headers
//

File avifile;
File idxfile;
#define AVIOFFSET 240 // AVI main header length
static int i = 0;
const int avi_header[AVIOFFSET] PROGMEM = {
  0x52, 0x49, 0x46, 0x46, 0xD8, 0x01, 0x0E, 0x00, 0x41, 0x56, 0x49, 0x20, 0x4C, 0x49, 0x53, 0x54,
  0xD0, 0x00, 0x00, 0x00, 0x68, 0x64, 0x72, 0x6C, 0x61, 0x76, 0x69, 0x68, 0x38, 0x00, 0x00, 0x00,
  0xA0, 0x86, 0x01, 0x00, 0x80, 0x66, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00,
  0x64, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x80, 0x02, 0x00, 0x00, 0xe0, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x4C, 0x49, 0x53, 0x54, 0x84, 0x00, 0x00, 0x00,
  0x73, 0x74, 0x72, 0x6C, 0x73, 0x74, 0x72, 0x68, 0x30, 0x00, 0x00, 0x00, 0x76, 0x69, 0x64, 0x73,
  0x4D, 0x4A, 0x50, 0x47, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x73, 0x74, 0x72, 0x66,
  0x28, 0x00, 0x00, 0x00, 0x28, 0x00, 0x00, 0x00, 0x80, 0x02, 0x00, 0x00, 0xe0, 0x01, 0x00, 0x00,
  0x01, 0x00, 0x18, 0x00, 0x4D, 0x4A, 0x50, 0x47, 0x00, 0x84, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x49, 0x4E, 0x46, 0x4F,
  0x10, 0x00, 0x00, 0x00, 0x6A, 0x61, 0x6D, 0x65, 0x73, 0x7A, 0x61, 0x68, 0x61, 0x72, 0x79, 0x20,
  0x76, 0x36, 0x30, 0x20, 0x4C, 0x49, 0x53, 0x54, 0x00, 0x01, 0x0E, 0x00, 0x6D, 0x6F, 0x76, 0x69,
};
int framesize = FRAMESIZE_SVGA;
#define BUFFSIZE 512
uint8_t buf[BUFFSIZE];
String avi_file_name;

struct frameSizeStruct {
  uint8_t frameWidth[2];
  uint8_t frameHeight[2];
};
static const frameSizeStruct frameSizeData[] = {
  {{0x60, 0x00}, {0x60, 0x00}}, // FRAMESIZE_96X96,    // 96x96
  {{0xA0, 0x00}, {0x78, 0x00}}, // FRAMESIZE_QQVGA,    // 160x120
  {{0xB0, 0x00}, {0x90, 0x00}}, // FRAMESIZE_QCIF,     // 176x144
  {{0xF0, 0x00}, {0xB0, 0x00}}, // FRAMESIZE_HQVGA,    // 240x176
  {{0xF0, 0x00}, {0xF0, 0x00}}, // FRAMESIZE_240X240,  // 240x240
  {{0x40, 0x01}, {0xF0, 0x00}}, // FRAMESIZE_QVGA,     // 320x240   framessize
  {{0x90, 0x01}, {0x28, 0x01}}, // FRAMESIZE_CIF,      // 400x296       bytes per buffer required in psram - quality must be higher number (lower quality) than config quality
  {{0xE0, 0x01}, {0x40, 0x01}}, // FRAMESIZE_HVGA,     // 480x320       low qual  med qual  high quality
  {{0x80, 0x02}, {0xE0, 0x01}}, // FRAMESIZE_VGA,      // 640x480   8   11+   ##  6-10  ##  0-5         indoor(56,COUNT=3)  (56,COUNT=2)          (56,count=1)
  //               38,400    61,440    153,600
  {{0x20, 0x03}, {0x58, 0x02}}, // FRAMESIZE_SVGA,     // 800x600   9                       240,000
  {{0x00, 0x04}, {0x00, 0x03}}, // FRAMESIZE_XGA,      // 1024x768  10
  {{0x00, 0x05}, {0xD0, 0x02}}, // FRAMESIZE_HD,       // 1280x720  11  115,200   184,320   460,800     (11)50.000  25.4fps   (11)50.000 12fps    (11)50,000  12.7fps
  {{0x00, 0x05}, {0x00, 0x04}}, // FRAMESIZE_SXGA,     // 1280x1024 12
  {{0x40, 0x06}, {0xB0, 0x04}}, // FRAMESIZE_UXGA,     // 1600x1200 13  240,000   384,000   960,000
  // 3MP Sensors
  {{0x80, 0x07}, {0x38, 0x04}}, // FRAMESIZE_FHD,      // 1920x1080 14  259,200   414,720   1,036,800   (11)210,000 5.91fps
  {{0xD0, 0x02}, {0x00, 0x05}}, // FRAMESIZE_P_HD,     //  720x1280 15
  {{0x60, 0x03}, {0x00, 0x06}}, // FRAMESIZE_P_3MP,    //  864x1536 16
  {{0x00, 0x08}, {0x00, 0x06}}, // FRAMESIZE_QXGA,     // 2048x1536 17  393,216   629,146   1,572,864
  // 5MP Sensors
  {{0x00, 0x0A}, {0xA0, 0x05}}, // FRAMESIZE_QHD,      // 2560x1440 18  460,800   737,280   1,843,200   (11)400,000 3.5fps    (11)330,000 1.95fps
  {{0x00, 0x0A}, {0x40, 0x06}}, // FRAMESIZE_WQXGA,    // 2560x1600 19
  {{0x38, 0x04}, {0x80, 0x07}}, // FRAMESIZE_P_FHD,    // 1080x1920 20
  {{0x00, 0x0A}, {0x80, 0x07}}  // FRAMESIZE_QSXGA,    // 2560x1920 21  614,400   983,040   2,457,600   (15)425,000 3.25fps   (15)382,000 1.7fps  (15)385,000 1.7fps

};
static void inline print_quartet(unsigned long i, File fd) {

  uint8_t y[4];
  y[0] = i % 0x100;
  y[1] = (i >> 8) % 0x100;
  y[2] = (i >> 16) % 0x100;
  y[3] = (i >> 24) % 0x100;
  size_t i1_err = fd.write(y , 4);
}

int frame_interval = 0;          // record at full speed
int speed_up_factor = 1;          // play at realtime
int avi_length = 1800;            // how long a movie in seconds -- 1800 sec = 30 min
uint32_t startms;
long totalp;
long totalw;

unsigned long movi_size = 0;
unsigned long jpeg_size = 0;
unsigned long idx_offset = 0;
uint32_t uVideoLen = 0;

int bad_jpg = 0;
int extend_jpg = 0;
int normal_jpg = 0;

long time_in_loop = 0;
long time_in_camera = 0;
long time_in_sd = 0;
long time_in_good = 0;
long time_total = 0;
long time_in_web1 = 0;
long time_in_web2 = 0;
long delay_wait_for_sd = 0;
long wait_for_cam = 0;

uint16_t remnant = 0;
#define fbs 8 // was 64 -- how many kb of static ram for psram -> sram buffer for sd write
uint8_t framebuffer_static[fbs * 1024 + 20];
static void inline print_2quartet(unsigned long i, unsigned long j, File fd) {

  uint8_t y[8];
  y[0] = i % 0x100;
  y[1] = (i >> 8) % 0x100;
  y[2] = (i >> 16) % 0x100;
  y[3] = (i >> 24) % 0x100;
  y[4] = j % 0x100;
  y[5] = (j >> 8) % 0x100;
  y[6] = (j >> 16) % 0x100;
  y[7] = (j >> 24) % 0x100;
  size_t i1_err = fd.write(y , 8);
}

int do_it_now = 0;
uint16_t frame_cnt = 0;
uint32_t elapsedms;

uint8_t zero_buf[4] = {0x00, 0x00, 0x00, 0x00};
uint8_t dc_buf[4] = {0x30, 0x30, 0x64, 0x63};    // "00dc"
uint8_t dc_and_zero_buf[8] = {0x30, 0x30, 0x64, 0x63, 0x00, 0x00, 0x00, 0x00};

uint8_t avi1_buf[4] = {0x41, 0x56, 0x49, 0x31};    // "AVI1"
uint8_t idx1_buf[4] = {0x69, 0x64, 0x78, 0x31};    // "idx1"

int gframe_cnt;
int gfblen;
int gj;
int  gmdelay;

TaskHandle_t the_camera_loop_task;
TaskHandle_t the_sd_loop_task;
void the_camera_loop (void* pvParameter);
int first = 1;
long frame_start = 0;
long frame_end = 0;
long frame_total = 0;
long frame_average = 0;
long loop_average = 0;
long loop_total = 0;
long total_frame_data = 0;
long last_frame_length = 0;
int done = 0;
long avi_start_time = 0;
long avi_end_time = 0;
int start_record = 0;
int start_record_2nd_opinion = -2;
int start_record_1st_opinion = -1;

int we_are_already_stopped = 0;
long total_delay = 0;
long bytes_before_last_100_frames = 0;
long time_before_last_100_frames = 0;

#define Lots_of_Stats 1
int quality = 12;

camera_fb_t * fb_curr = NULL;
camera_fb_t * fb_next = NULL;
SemaphoreHandle_t baton;
int framebuffer_len;
int framebuffer2_len;
int framebuffer3_len;
long framebuffer_time = 0;
long framebuffer2_time = 0;
long framebuffer3_time = 0;
uint8_t* framebuffer;
uint8_t* framebuffer2;
uint8_t* framebuffer3;
static SemaphoreHandle_t sd_go;
#define blinking 0
bool restart_now = false;
bool reboot_now = false;

static SemaphoreHandle_t wait_for_sd;
long current_frame_time;
long last_frame_time;
float most_recent_fps = 0;
int most_recent_avg_framesize = 0;


camera_fb_t *  get_good_jpeg() {

  camera_fb_t * fb;

  long start;
  int failures = 0;

  do {
    int fblen = 0;
    int foundffd9 = 0;
    long bp = millis();
    long mstart = micros();

    fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("Camera Capture Failed");
      failures++;
    } else {
      long mdelay = micros() - mstart;

      int get_fail = 0;

      totalp = totalp + millis() - bp;
      time_in_camera = totalp;

      fblen = fb->len;

      for (int j = 1; j <= 1025; j++) {
        if (fb->buf[fblen - j] != 0xD9) {
          // no d9, try next for
        } else {                                     //Serial.println("Found a D9");
          if (fb->buf[fblen - j - 1] == 0xFF ) {     //Serial.print("Found the FFD9, junk is "); Serial.println(j);
            if (j == 1) {
              normal_jpg++;
            } else {
              extend_jpg++;
            }
            foundffd9 = 1;
            if (Lots_of_Stats) {
              if (j > 900) {                             //  rarely happens - sometimes on 2640
                Serial.print("Frame "); Serial.print(frame_cnt);
                Serial.print(", Len = "); Serial.print(fblen);
                //Serial.print(", Correct Len = "); Serial.print(fblen - j + 1);
                Serial.print(", Extra Bytes = "); Serial.println( j - 1);
              }

              if ( frame_cnt % 100 == 50) {
                gframe_cnt = frame_cnt;
                gfblen = fblen;
                gj = j;
                gmdelay = mdelay;
                //Serial.printf("Frame %6d, len %6d, extra  %4d, cam time %7d ", frame_cnt, fblen, j - 1, mdelay / 1000);
                //logfile.printf("Frame %6d, len %6d, extra  %4d, cam time %7d ", frame_cnt, fblen, j - 1, mdelay / 1000);
                do_it_now = 1;
              }
            }
            break;
          }
        }
      }

      if (!foundffd9) {
        bad_jpg++;
        Serial.printf("Bad jpeg, Frame %d, Len = %d \n", frame_cnt, fblen);
        esp_camera_fb_return(fb);
        failures++;

      } else {
        break;
        // count up the useless bytes
      }
    }

  } while (failures < 10);   // normally leave the loop with a break()

  // if we get 10 bad frames in a row, then quality parameters are too high - set them lower (+5), and start new movie
  if (failures == 10) {
    Serial.printf("10 failures");

    sensor_t * ss = esp_camera_sensor_get();
    int qual = ss->status.quality ;
    ss->set_quality(ss, qual + 5);
    quality = qual + 5;
    Serial.printf("\n\nDecreasing quality due to frame failures %d -> %d\n\n", qual, qual + 5);
    delay(1000);

    start_record = 0;
    //reboot_now = true;
  }
  return fb;
}



static void start_avi() {

  long start = millis();

  Serial.println("Starting an avi ");

  avi_file_name = "avi_" + Count.getCount() + ".avi";

  avifile = SD_MMC.open(avi_file_name, "w");
  idxfile = SD_MMC.open("/idx.tmp", "w");

  if (avifile) {
    Serial.printf("File open: %s\n", avi_file_name);
  }  else  {
    Serial.println("Could not open file");
    return;
  }

  if (idxfile)  {
    //Serial.printf("File open: %s\n", "//idx.tmp");
  }  else  {
    Serial.println("Could not open file /idx.tmp");
    return;
  }

  for ( i = 0; i < AVIOFFSET; i++) {
    char ch = pgm_read_byte(&avi_header[i]);
    buf[i] = ch;
  }

  memcpy(buf + 0x40, frameSizeData[framesize].frameWidth, 2);
  memcpy(buf + 0xA8, frameSizeData[framesize].frameWidth, 2);
  memcpy(buf + 0x44, frameSizeData[framesize].frameHeight, 2);
  memcpy(buf + 0xAC, frameSizeData[framesize].frameHeight, 2);

  size_t err = avifile.write(buf, AVIOFFSET);

  uint8_t ex_fps = 1;
  if (frame_interval == 0) {
    if (framesize >= 11) {
      ex_fps = 12.5 * speed_up_factor ;;
    } else {
      ex_fps = 25.0 * speed_up_factor;
    }
  } else {
    ex_fps = round(1000.0 / frame_interval * speed_up_factor);
  }

  avifile.seek( 0x84 , SeekSet);
  print_quartet((int)ex_fps, avifile);

  avifile.seek( AVIOFFSET, SeekSet);

  Serial.print(F("\nRecording "));
  Serial.print(avi_length);
  Serial.println(" seconds.");

  startms = millis();

  totalp = 0;
  totalw = 0;

  jpeg_size = 0;
  movi_size = 0;
  uVideoLen = 0;
  idx_offset = 4;

  bad_jpg = 0;
  extend_jpg = 0;
  normal_jpg = 0;

  time_in_loop = 0;
  time_in_camera = 0;
  time_in_sd = 0;
  time_in_good = 0;
  time_total = 0;
  time_in_web1 = 0;
  time_in_web2 = 0;
  delay_wait_for_sd = 0;
  wait_for_cam = 0;

  time_in_sd += (millis() - start);

  avifile.flush();

} // end of start avi

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//  another_save_avi saves another frame to the avi file, uodates index
//           -- pass in a fb pointer to the frame to add
//

static void another_save_avi(camera_fb_t * fb) {

  camera_fb_t * fb = get_good_jpeg();

  long start = millis();

  int fblen;
  fblen = fb->len;

  int fb_block_length;
  uint8_t* fb_block_start;

  jpeg_size = fblen;

  remnant = (4 - (jpeg_size & 0x00000003)) & 0x00000003;

  long bw = millis();
  long frame_write_start = millis();

  framebuffer_static[0] = 0x30;       // "00dc"
  framebuffer_static[1] = 0x30;
  framebuffer_static[2] = 0x64;
  framebuffer_static[3] = 0x63;

  int jpeg_size_rem = jpeg_size + remnant;

  framebuffer_static[4] = jpeg_size_rem % 0x100;
  framebuffer_static[5] = (jpeg_size_rem >> 8) % 0x100;
  framebuffer_static[6] = (jpeg_size_rem >> 16) % 0x100;
  framebuffer_static[7] = (jpeg_size_rem >> 24) % 0x100;

  fb_block_start = fb->buf;

  if (fblen > fbs * 1024 - 8 ) {                     // fbs is the size of frame buffer static
    fb_block_length = fbs * 1024;
    fblen = fblen - (fbs * 1024 - 8);
    memcpy(framebuffer_static + 8, fb_block_start, fb_block_length - 8);
    fb_block_start = fb_block_start + fb_block_length - 8;

  } else {
    fb_block_length = fblen + 8  + remnant;
    memcpy(framebuffer_static + 8, fb_block_start,  fblen);
    fblen = 0;
  }

  size_t err = avifile.write(framebuffer_static, fb_block_length);

  if (err != fb_block_length) {
    Serial.print("Error on avi write: err = "); Serial.print(err);
    Serial.print(" len = "); Serial.println(fb_block_length);
  }

  while (fblen > 0) {

    if (fblen > fbs * 1024) {
      fb_block_length = fbs * 1024;
      fblen = fblen - fb_block_length;
    } else {
      fb_block_length = fblen  + remnant;
      fblen = 0;
    }

    memcpy(framebuffer_static, fb_block_start, fb_block_length);

    size_t err = avifile.write(framebuffer_static,  fb_block_length);

    if (err != fb_block_length) {
      Serial.print("Error on avi write: err = "); Serial.print(err);
      Serial.print(" len = "); Serial.println(fb_block_length);
    }

    fb_block_start = fb_block_start + fb_block_length;
    delay(0);
  }


  movi_size += jpeg_size;
  uVideoLen += jpeg_size;
  long frame_write_end = millis();

  print_2quartet(idx_offset, jpeg_size, idxfile);

  idx_offset = idx_offset + jpeg_size + remnant + 8;

  movi_size = movi_size + remnant;

  if ( do_it_now == 1 && frame_cnt < 1011) {
    do_it_now = 0;
    Serial.printf("Frame %6d, len %6d, extra  %4d, cam time %7d,  sd time %4d -- \n", gframe_cnt, gfblen, gj - 1, gmdelay / 1000, millis() - bw);
  }

  totalw = totalw + millis() - bw;
  time_in_sd += (millis() - start);

  avifile.flush();


} // end of another_pic_avi

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//  end_avi writes the index, and closes the files
//

static void end_avi() {

  long start = millis();

  unsigned long current_end = avifile.position();

  Serial.println("End of avi - closing the files");

  if (frame_cnt <  5 ) {
    Serial.println("Recording screwed up, less than 5 frames, forget index\n");
    idxfile.close();
    avifile.close();
    int xx = remove("/idx.tmp");
    int yy = remove(avi_file_name.c_str());

  } else {

    elapsedms = millis() - startms;

    float fRealFPS = (1000.0f * (float)frame_cnt) / ((float)elapsedms) * speed_up_factor;

    float fmicroseconds_per_frame = 1000000.0f / fRealFPS;
    uint8_t iAttainedFPS = round(fRealFPS) ;
    uint32_t us_per_frame = round(fmicroseconds_per_frame);

    //Modify the MJPEG header from the beginning of the file, overwriting various placeholders

    avifile.seek( 4 , SeekSet);
    print_quartet(movi_size + 240 + 16 * frame_cnt + 8 * frame_cnt, avifile);

    avifile.seek( 0x20 , SeekSet);
    print_quartet(us_per_frame, avifile);

    unsigned long max_bytes_per_sec = (1.0f * movi_size * iAttainedFPS) / frame_cnt;

    avifile.seek( 0x24 , SeekSet);
    print_quartet(max_bytes_per_sec, avifile);

    avifile.seek( 0x30 , SeekSet);
    print_quartet(frame_cnt, avifile);

    avifile.seek( 0x8c , SeekSet);
    print_quartet(frame_cnt, avifile);

    avifile.seek( 0x84 , SeekSet);
    print_quartet((int)iAttainedFPS, avifile);

    avifile.seek( 0xe8 , SeekSet);
    print_quartet(movi_size + frame_cnt * 8 + 4, avifile);

    Serial.println(F("\n*** Video recorded and saved ***\n"));

    Serial.printf("Recorded %5d frames in %5d seconds\n", frame_cnt, elapsedms / 1000);
    Serial.printf("File size is %u bytes\n", movi_size + 12 * frame_cnt + 4);
    Serial.printf("Adjusted FPS is %5.2f\n", fRealFPS);
    Serial.printf("Max data rate is %lu bytes/s\n", max_bytes_per_sec);
    Serial.printf("Frame duration is %d us\n", us_per_frame);
    Serial.printf("Average frame length is %d bytes\n", uVideoLen / frame_cnt);
    Serial.print("Average picture time (ms) "); Serial.println( 1.0 * totalp / frame_cnt);
    Serial.print("Average write time (ms)   "); Serial.println( 1.0 * totalw / frame_cnt );
    Serial.print("Normal jpg % ");  Serial.println( 100.0 * normal_jpg / frame_cnt, 1 );
    Serial.print("Extend jpg % ");  Serial.println( 100.0 * extend_jpg / frame_cnt, 1 );
    Serial.print("Bad    jpg % ");  Serial.println( 100.0 * bad_jpg / frame_cnt, 5 );

    Serial.printf("Writng the index, %d frames\n", frame_cnt);

    avifile.seek( current_end , SeekSet);

    idxfile.close();

    size_t i1_err = avifile.write(idx1_buf, 4);

    print_quartet(frame_cnt * 16, avifile);

    idxfile = SD_MMC.open("/idx.tmp", "r");

    if (idxfile)  {
    }  else  {
      Serial.println("Could not open index file");
      return;
    }

    char * AteBytes;
    AteBytes = (char*) malloc (8);

    for (int i = 0; i < frame_cnt; i++) {
      size_t res = idxfile.readBytes( AteBytes, 8);
      size_t i1_err = avifile.write(dc_buf, 4);
      size_t i2_err = avifile.write(zero_buf, 4);
      size_t i3_err = avifile.write((uint8_t *)AteBytes, 8);
    }

    free(AteBytes);

    idxfile.close();
    avifile.close();

    int xx = SD_MMC.remove("/idx.tmp");
  }

  Serial.println("---");
  time_in_sd += (millis() - start);

  Serial.println("");
  time_total = millis() - startms;
  Serial.printf("waiting for cam %10dms, %4.1f%%\n", wait_for_cam , 100.0 * wait_for_cam  / time_total);
  Serial.printf("Time in camera  %10dms, %4.1f%%\n", time_in_camera, 100.0 * time_in_camera / time_total);
  Serial.printf("waiting for sd  %10dms, %4.1f%%\n", delay_wait_for_sd , 100.0 * delay_wait_for_sd  / time_total);
  Serial.printf("Time in sd      %10dms, %4.1f%%\n", time_in_sd    , 100.0 * time_in_sd     / time_total);
  Serial.printf("web (core 1)    %10dms, %4.1f%%\n", time_in_web1  , 100.0 * time_in_web1   / time_total);
  Serial.printf("web (core 0)    %10dms, %4.1f%%\n", time_in_web2  , 100.0 * time_in_web2   / time_total);
  Serial.printf("time total      %10dms, %4.1f%%\n", time_total    , 100.0 * time_total     / time_total);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


void takeNewPhoto(){

  // Path where new picture will be saved in SD Card
  String path = "/image" + Count.getCount() + ".jpg";
  
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

  Serial.println("saving...");
  file.write(fb->buf, fb->len); // payload (image), payload length
  // Close the file when done
  file.close();

  // Return the frame buffer back to the driver for reuse
  esp_camera_fb_return(fb);
  // Update picture number counter
  Count.setNextCount();

  Serial.printf("Saved file to path: %s\n", path.c_str());
}


// the camera require few image capture to calibrate exposure, white balance, etc
void camCalibrate(int duration){
  Serial.println("Calibrating...");
  unsigned long now = millis();
  while (now <= duration){
    now = millis();
    if (now % 200 == 0){
      camera_fb_t *fb = esp_camera_fb_get();
      if (!fb){
        Serial.println("Cab failed");
        return;
      }

      esp_camera_fb_return(fb);
      Serial.println(".");
    }
    delay(1);
  }
  Serial.println("\nCab done");
}

OneButton btn = OneButton(
BUTTON_PIN,  // Input pin for the button
true,        // Button is active LOW
true         // Enable internal pull-up resistor
);

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

  camCalibrate(6000);

  // Initialize the MicroSD
  Serial.print("Initializing the MicroSD card module... ");
  initMicroSDCard();

  wait_for_sd = xSemaphoreCreateBinary(); //xSemaphoreCreateMutex();
  sd_go = xSemaphoreCreateBinary(); //xSemaphoreCreateMutex();
  baton = xSemaphoreCreateMutex();


  xTaskCreatePinnedToCore( the_camera_loop, "the_camera_loop", 3000, NULL, 6, &the_camera_loop_task, 0); // prio 3, core 0 //v56 core 1 as http dominating 0 ... back to 0, raise prio
  delay(100);
  xTaskCreatePinnedToCore( the_sd_loop, "the_sd_loop", 2000, NULL, 4, &the_sd_loop_task, 1);  // prio 4, core 1

  btn.attachClick(takeNewPhoto);
  btn.attachLongPressStart(start_avi);
  btn.attachLongPressStop(end_avi2);
}

int delete_old_stuff_flag = 0;

void end_avi2(){
      Serial.println("End the Avi");
      restart_now = false;

      xSemaphoreTake( wait_for_sd, portMAX_DELAY );
      esp_camera_fb_return(fb_curr);

      frame_cnt++;
      fb_curr = fb_next;
      fb_next = NULL;

      xSemaphoreGive( sd_go );                  // save final frame of movie

      if (blinking)  digitalWrite(33, frame_cnt % 2);

      xSemaphoreTake( wait_for_sd, portMAX_DELAY );    // wait for final frame of movie to be written

      esp_camera_fb_return(fb_curr);
      fb_curr = NULL;

      end_avi();                                // end the movie

      if (blinking) digitalWrite(33, HIGH);          // light off

      delete_old_stuff_flag = 1;
      delay(50);

      avi_end_time = millis();

      float fps = 1.0 * frame_cnt / ((avi_end_time - avi_start_time) / 1000) ;

      Serial.printf("End the avi at %d.  It was %d frames, %d ms at %.2f fps...\n", millis(), frame_cnt, avi_end_time, avi_end_time - avi_start_time, fps);

      if (!reboot_now) frame_cnt = 0;             // start recording again on the next loop
}

void the_camera_loop (void* pvParameter) {

  Serial.print("the camera loop, core ");  Serial.print(xPortGetCoreID());
  Serial.print(", priority = "); Serial.println(uxTaskPriorityGet(NULL));

  frame_cnt = 0;
  start_record_2nd_opinion = digitalRead(12);
  start_record_1st_opinion = digitalRead(12);
  start_record = 0;

  delay(1000);

  while (1) {

    // if (frame_cnt == 0 && start_record == 0)  // do nothing
    // if (frame_cnt == 0 && start_record == 1)  // start a movie
    // if (frame_cnt > 0 && start_record == 0)   // stop the movie
    // if (frame_cnt > 0 && start_record != 0)   // another frame

    ///////////////////  NOTHING TO DO //////////////////
    if (frame_cnt == 0 && start_record == 0) {

      // Serial.println("Do nothing");
      if (we_are_already_stopped == 0) Serial.println("\n\nDisconnect Pin 12 from GND to start recording.\n\n");
      we_are_already_stopped = 1;
      delay(100);

      ///////////////////  START A MOVIE  //////////////////
    } else if (frame_cnt == 0 && start_record == 1) {

      //Serial.println("Ready to start");

      we_are_already_stopped = 0;

      //delete_old_stuff(); // move to loop

      avi_start_time = millis();
      Serial.printf("\nStart the avi ... at %d\n", avi_start_time);
      Serial.printf("Framesize %d, quality %d, length %d seconds\n\n", framesize, quality, avi_length);

      frame_cnt++;

      long wait_for_cam_start = millis();
      fb_curr = get_good_jpeg();                     // should take zero time
      wait_for_cam += millis() - wait_for_cam_start;

      //start_avi(); will be replaced with semaphore taken when button push

      wait_for_cam_start = millis();
      fb_next = get_good_jpeg();                    // should take nearly zero time due to time spent writing header
      //if (framebuffer_time < (millis() - 10)){
      xSemaphoreTake( baton, portMAX_DELAY );
      framebuffer_len = fb_next->len;                    // v59.5
      memcpy(framebuffer, fb_next->buf, fb_next->len);   // v59.5
      framebuffer_time = millis();                       // v59.5
      xSemaphoreGive( baton );
      //}
      wait_for_cam += millis() - wait_for_cam_start;
      xSemaphoreGive( sd_go );                     // trigger sd write to write first frame

      if (blinking) digitalWrite(33, frame_cnt % 2);                // blink

      ///////////////////  END THE MOVIE //////////////////

        // moved to end_avi2()

      ///////////////////  ANOTHER FRAME  //////////////////
    } else if (frame_cnt > 0 && start_record != 0) {  // another frame of the avi

      //Serial.println("Another frame");

      current_frame_time = millis();
      if (current_frame_time - last_frame_time < frame_interval) {
        delay(frame_interval - (current_frame_time - last_frame_time));             // delay for timelapse
      }
      last_frame_time = millis();

      frame_cnt++;

      long delay_wait_for_sd_start = millis();
      xSemaphoreTake( wait_for_sd, portMAX_DELAY );             // make sure sd writer is done
      delay_wait_for_sd += millis() - delay_wait_for_sd_start;

      esp_camera_fb_return(fb_curr);

      fb_curr = fb_next;           // we will write a frame, and get the camera preparing a new one

      xSemaphoreGive( sd_go );             // write the frame in fb_curr

      long wait_for_cam_start = millis();
      fb_next = get_good_jpeg();               // should take near zero, unless the sd is faster than the camera, when we will have to wait for the camera
      //if (framebuffer_time < (millis() - 10)){
      xSemaphoreTake( baton, portMAX_DELAY );
      framebuffer_len = fb_next->len;                    // v59.5
      memcpy(framebuffer, fb_next->buf, fb_next->len);   // v59.5
      framebuffer_time = millis();                       // v59.5
      xSemaphoreGive( baton );
      //}
      wait_for_cam += millis() - wait_for_cam_start;

      if (blinking) digitalWrite(33, frame_cnt % 2);

      if (frame_cnt % 100 == 10 ) {     // print some status every 100 frames
        if (frame_cnt == 10) {
          bytes_before_last_100_frames = movi_size;
          time_before_last_100_frames = millis();
          most_recent_fps = 0;
          most_recent_avg_framesize = 0;
        } else {

          most_recent_fps = 100.0 / ((millis() - time_before_last_100_frames) / 1000.0) ;
          most_recent_avg_framesize = (movi_size - bytes_before_last_100_frames) / 100;

          if (Lots_of_Stats && frame_cnt < 1011) {
            Serial.printf("So far: %04d frames, in %6.1f seconds, for last 100 frames: avg frame size %6.1f kb, %.2f fps ...\n", frame_cnt, 0.001 * (millis() - avi_start_time), 1.0 / 1024  * most_recent_avg_framesize, most_recent_fps);
          }

          total_delay = 0;

          bytes_before_last_100_frames = movi_size;
          time_before_last_100_frames = millis();
        }
      }
    }
  }
}

void the_sd_loop (void* pvParameter) {

  Serial.print("the_sd_loop, core ");  Serial.print(xPortGetCoreID());
  Serial.print(", priority = "); Serial.println(uxTaskPriorityGet(NULL));

  while (1) {
    xSemaphoreTake( sd_go, portMAX_DELAY );            // we wait for camera loop to tell us to go
    another_save_avi(fb_curr);                        // do the actual sd wrte
    xSemaphoreGive( wait_for_sd );                     // tell camera loop we are done
  }
}

void loop()
{
  btn.tick();
  delay(1);
}