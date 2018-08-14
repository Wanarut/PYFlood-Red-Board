// Date, Time and Alarm functions using a DS3231 RTC connected via I2C and Wire lib
#include "Wire.h"
#include "SPI.h"  // not used here, but needed to prevent a RTClib compile error
#include "RTClib.h"

RTC_DS3231 RTC;

void setup (){
  Wire.begin();
  RTC.begin();
  RTC.adjust(DateTime(__DATE__, __TIME__));
  
  /*uint16_t year = 2018;
  uint8_t month = 8;
  uint8_t day = 14;
  uint8_t hour = 23;
  uint8_t min = 58;
  uint8_t sec = 0;
  RTC.adjust(DateTime (year, month, day, hour , min, sec));*/
}

void loop (){
  
}
