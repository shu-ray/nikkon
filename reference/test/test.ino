#include "FS.h"               // SD Card ESP32
#include "SD_MMC.h"           // SD Card ESP32

const char * countPath = "/camSettings/picIndex.txt";
int data = 0;

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


void setup() {
  Serial.begin(115200);
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
  printFile(countPath,1635);
  data = readFile(countPath).toInt();
}


void loop()
{
  Serial.println(data);
  delay(500);
}
