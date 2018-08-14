//Input
#include<Wire.h>  //สั่ง Include Library สำหรับ I2C (ไฟล์ Wire.h)
#include <SPI.h>  // not used here, but needed to prevent a RTClib compile error
#include <RTClib.h>
#include <TSL2561.h>
//Output
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <SoftwareSerial.h>
//Process
#include <ArduinoJson.h>
//Chat with Slave
SoftwareSerial chat(10, 11); // RX, TX
//I2C
RTC_DS3231 RTC;
TSL2561 tsl(TSL2561_ADDR_FLOAT);
Adafruit_SSD1306 OLED(-1);
//JSON
StaticJsonBuffer<200> jsonBuffer;
JsonObject& data = jsonBuffer.createObject();

String StationID = "302"; //<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< Station ID

/////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////                                                       /////////////////////
/////////////////////                     DECLARE  PINs                     /////////////////////
/////////////////////                                                       /////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////
int UltrasonicPin = A0;  
int RainPin = A1;
int VoltPin = A2;
int TemperaturePin = A3;
int HumidityPin = A4;

/////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////                                                       /////////////////////
/////////////////////               DECLARE  GLOBAL VARIABLEs               /////////////////////
/////////////////////                                                       /////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////
int day, month, year, hour, minute, second, status;
int light, ultrasonic, rain, volt, temperature_out, humidity;
float temperature_in;

/////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////                                                       /////////////////////
/////////////////////                         SETUP                         /////////////////////
/////////////////////                                                       /////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////
void setup(){
  Serial.begin(9600);  // start serial for output
  chat.begin(4800);
  Wire.begin();        // join i2c bus (address optional for master)
  //Real Time Clock
  RTC.begin();
  if(!RTC.isrunning()){
    Serial.println("RTC is NOT running!");
    // following line sets the RTC to the date & time this sketch was compiled
    RTC.adjust(DateTime(__DATE__, __TIME__));
  }else{
    Serial.println("Found RTC");
  }
  //Light Sensor
  if(tsl.begin()){
    Serial.println("Found Light Sensor");
  }else{
    Serial.println("No Light sensor?");
  }
  //OLED Screen
  OLED.begin(SSD1306_SWITCHCAPVCC,0x3C); // initialize with the I2C addr 0x3C (for the 128x64)
  OLED.setRotation(3);
  showLOGO();

  // You can change the gain on the fly, to adapt to brighter/dimmer light situations
  //tsl.setGain(TSL2561_GAIN_0X);         // set no gain (for bright situtations)
  tsl.setGain(TSL2561_GAIN_16X);      // set 16x gain (for dim situations)
  
  // Changing the integration time gives you a longer time over which to sense light
  // longer timelines are slower, but are good in very low light situtations!
  tsl.setTiming(TSL2561_INTEGRATIONTIME_13MS);  // shortest integration time (bright light)
  //tsl.setTiming(TSL2561_INTEGRATIONTIME_101MS);  // medium integration time (medium light)
  //tsl.setTiming(TSL2561_INTEGRATIONTIME_402MS);  // longest integration time (dim light)

  JsonArray& date = data.createNestedArray("date");
  JsonArray& time = data.createNestedArray("time");
  
  DateTime now = RTC.now();
  
  date.add(now.day());
  date.add(now.month());
  date.add(now.year());
  
  time.add(now.hour());
  time.add(now.minute());
  time.add(now.second());

  data["status"] = 255;

  data["temp"] = RTC.getTemperature();

  JsonArray& analog = data.createNestedArray("analog");
  analog.add(analogRead(UltrasonicPin));
  analog.add(analogRead(RainPin));
  analog.add(analogRead(VoltPin));
  analog.add(analogRead(TemperaturePin));
  analog.add(analogRead(HumidityPin));

  JsonArray& light = data.createNestedArray("light");
  uint16_t x = tsl.getLuminosity(TSL2561_VISIBLE);     
  uint32_t lum = tsl.getFullLuminosity();
  uint16_t ir, full;
  ir = lum >> 16;
  full = lum & 0xFFFF;
  
  light.add(x);
  light.add(ir);
  light.add(full);
  light.add(full - ir);
  light.add(tsl.calculateLux(full, ir));
}

/////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////                                                       /////////////////////
/////////////////////                         LOOPs                         /////////////////////
/////////////////////                                                       /////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////
bool flag = true;
void loop(){
  getRTCdata();
  getANLdata();
  getTSLdata();
  WriteToGlobal();
  showOLED();

  String output;
  data.printTo(output);
  Serial.println(output);

  DateTime now = RTC.now();
  int second = now.second();
  int minute = now.minute();
  if (minute % 5 == 0 && second == 0 && flag){
    Serial.println();
    String link = CreateLink();
    SendlinkToSlave(link);
    Serial.println();
    flag = false;
  }else{
    flag = true;
  }

  delay(200);
}

/////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////                                                       /////////////////////
/////////////////////                       GET DATUM                       /////////////////////
/////////////////////                                                       /////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////
void getRTCdata(){  
  DateTime now = RTC.now();
  
  data["date"][0] = now.day();
  data["date"][1] = now.month();
  data["date"][2] = now.year();
  
  data["time"][0] = now.hour();
  data["time"][1] = now.minute();
  data["time"][2] = now.second();
  
  data["status"] = 255;
  
  data["temp"] = RTC.getTemperature();
}

void getANLdata(){
  getUltrasonic();
  data["analog"][1] = analogRead(RainPin);
  data["analog"][2] = analogRead(VoltPin);
  data["analog"][3] = analogRead(TemperaturePin);
  data["analog"][4] = analogRead(HumidityPin);
}

void getUltrasonic(){
  int i;
  float ewma_sum = 0;
  for (i = 0; i <= 30; i++)
  {
    int val_ultrasonic = analogRead(UltrasonicPin);
    ewma_sum += val_ultrasonic * 2;
  }
  data["analog"][3] = (ewma_sum / 30);
}

void getTSLdata(){
  //String s_luminosity, s_ir, s_full, s_visible, s_lux;
  // Simple data read example. Just read the infrared, fullspecrtrum diode 
  // or 'visible' (difference between the two) channels.
  // This can take 13-402 milliseconds! Uncomment whichever of the following you want to read
  uint16_t x = tsl.getLuminosity(TSL2561_VISIBLE);     
  //uint16_t x = tsl.getLuminosity(TSL2561_FULLSPECTRUM);
  //uint16_t x = tsl.getLuminosity(TSL2561_INFRARED);
  
  //data["lumi"] = x;
  data["light"][0] = x;
  //Serial.println(x, DEC);

  // More advanced data read example. Read 32 bits with top 16 bits IR, bottom 16 bits full spectrum
  // That way you can do whatever math and comparisons you want!
  uint32_t lum = tsl.getFullLuminosity();
  uint16_t ir, full;
  ir = lum >> 16;
  full = lum & 0xFFFF;
  
  //data["ir"] = ir;
  data["light"][1] = ir;
  //Serial.print("IR: "); Serial.print(ir);   Serial.print("\t\t");
  //data["full"] = full;
  data["light"][2] = full;
  //Serial.print("Full: "); Serial.print(full);   Serial.print("\t");
  //data["visi"] = full - ir;
  data["light"][3] = (full - ir);
  //Serial.print("Visible: "); Serial.print(full - ir);   Serial.print("\t");
  //data["lux"] = tsl.calculateLux(full, ir);
  data["light"][4] = tsl.calculateLux(full, ir);
  //Serial.print("Lux: "); Serial.println(tsl.calculateLux(full, ir));
}

/////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////                                                       /////////////////////
/////////////////////                      OLED SCREEN                      /////////////////////
/////////////////////                                                       /////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////
void showLOGO(){
  OLED.clearDisplay(); 
  OLED.setTextColor(WHITE,BLACK);   //Text is white ,background is black
  OLED.setCursor(0, 0);
  OLED.setTextSize(2);
  OLED.println("OASYS");
  OLED.setTextSize(1);
  OLED.println("sync data");
  OLED.display();
  delay(2000);
  OLED.clearDisplay();
}

void showOLED(){
  OLED.clearDisplay(); 
  OLED.setTextColor(WHITE,BLACK);   //Text is white ,background is black
  OLED.setCursor(0, 0);
  OLED.setTextSize(1);
  OLED.print("Phayao");
  OLED.setTextSize(2);
  OLED.println("QQ");
  OLED.setTextSize(1);
  
  OLED.print(day);    OLED.print("/"); OLED.print(month);   OLED.print("/"); OLED.println(year);
  OLED.print(hour);   OLED.print(":"); OLED.print(minute);  OLED.print(":"); OLED.println(second);
  OLED.print("Ti:");  OLED.print(temperature_in);  OLED.println(" C");

  OLED.println();
  OLED.print("Status:"); 
  if(status == 255){
    OLED.println("ON");
  }else{
    OLED.println("OFF");
  }
  
  OLED.println();
  OLED.print("Ultra:"); OLED.println(ultrasonic);
  OLED.print("Rain:"); OLED.println(rain);
  OLED.print("Volt:"); OLED.println(volt);
  OLED.print("Lumino:"); OLED.println(light);
  OLED.print("To:"); OLED.println(temperature_out);
  OLED.print("Hum:"); OLED.println(humidity);
  
  /*OLED.print("IR:"); OLED.println((int)data["light"][1]);
  OLED.print("Full:"); OLED.println((int)data["light"][2]);
  OLED.print("Vcble:"); OLED.println((int)data["light"][3]);
  OLED.print("Lux:"); OLED.println((int)data["light"][4]);*/
  OLED.display(); 
}

/////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////                                                       /////////////////////
/////////////////////            WRITE VALUE TO GLOBAL VARIABLEs            /////////////////////
/////////////////////                                                       /////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////
void WriteToGlobal(){
  day = data["date"][0];
  month = data["date"][1];
  year = data["date"][2];
  
  hour = data["time"][0];
  minute = data["time"][1];
  second = data["time"][2];

  temperature_in = data["temp"];
  status = data["status"];
  
  light = data["light"][0];
  ultrasonic = data["analog"][0];
  rain = data["analog"][1];
  volt = data["analog"][2];
  temperature_out = data["analog"][3];
  humidity = data["analog"][4];
}

/////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////                                                       /////////////////////
/////////////////////                 CREATE LINK for SLAVE                 /////////////////////
/////////////////////                                                       /////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////
String CreateLink(){
  String URL = "http://www.thaiwaterqq.com/api-qq/send_db_quality.php?";
  String field1 = "&StationID=" + StationID;
  String field2 = "&D_O=" + (String)("%.2f",day);
  String field3 = "&pH=" + (String)("%.2f",hour);
  String field4 = "&Conductivity=" + (String)("%.2f",status);
  String field5 = "&temp=" + (String)("%.2f",temperature_in);
  
  String field6 = "&D_O_2=" + (String)("%.2f",light);
  String field7 = "&pH_2=" + (String)("%.2f",ultrasonic);
  String field8 = "&Conductivity_2=" + (String)("%.2f",rain);
  String field9 = "&temp_2=" + (String)("%.2f",volt);
  
  String SendData_string = URL + field1 + field2 + field3 + field4 + field5 + field6 + field7 + field8 + field9;
  return SendData_string;
}

void SendlinkToSlave(String SendData_string){
  Serial.println("-----------------------------------------SEND DATA QUALITY-----------------------------------------");
  Serial.println(SendData_string);
  chat.print(SendData_string);
}

