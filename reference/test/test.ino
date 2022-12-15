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

TouchSensor camButton(12, 30);
void setup()
{
  Serial.begin(115200);
  delay(1000); // give me time to bring up serial monitor
  Serial.println("ESP32 Touch Test");
}

void loop()
{
  if (camButton.just_pressed()){
    Serial.println("yey");
  }
  //Serial.println(touchRead(pin));
 delay(100);
}
