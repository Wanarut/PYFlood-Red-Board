/////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////                                                       /////////////////////
/////////////////////                      STATION  ID                      /////////////////////
/////////////////////                                                       /////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////
String StationID = "5"; //<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< Station ID
String StationStatus = "ON";

int Button1Pin = 12;      //Button1
int Button2Pin = 13;      //Button2

#define VALUE_SIZE 20     //Read Value to analys
#define TEST_PACKAGE 45
#define delaytime 100
#define magicNumber 1.68  //MagicNumber for Phynics Board
float u_value[VALUE_SIZE];  //Array Value
float prev_value = 0;
float alpha = 0.8;

//Input
#include<Wire.h>  //สั่ง Include Library สำหรับ I2C (ไฟล์ Wire.h)
#include <RTClib.h>
#include <TSL2561.h>
#include <SparkFunHTU21D.h>
#include <Adafruit_BME280.h>
#include <DHT12.h>
//Output
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <SoftwareSerial.h>
//Process
#include <ArduinoJson.h>
#include <QuickStats.h>
//Chat with Slave
SoftwareSerial chat(10, 11); // RX, TX
bool GSMStart = false;
String SignalQuality = "o";
//SPI
#include <SPI.h>  // not used here, but needed to prevent a RTClib compile error
#include <SD.h>
//JSON
StaticJsonBuffer<300> jsonBuffer;
JsonObject& data = jsonBuffer.createObject();
//Stat
QuickStats stats; //initialize an instance of this class
//Battery
#define ARDUINO_WORK_VOLTAGE 4.0
//I2C
RTC_DS3231 RTC;
bool hasRTC = false;

TSL2561 TSL2561(TSL2561_ADDR_FLOAT);
bool hasTSL = false;
bool hasUV = false;

HTU21D Humidity;
bool hasHTU = false;

Adafruit_BME280 BME280;
#define SEALEVELPRESSURE_HPA (1013.25)
bool hasBME = false;

DHT12 dht12;
bool hasDHT = false;

Adafruit_SSD1306 OLED(-1);

/////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////                                                       /////////////////////
/////////////////////                     DECLARE  PINs                     /////////////////////
/////////////////////                                                       /////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////
int PIN_REF_3V3 = A0;     //3.3V Reference UV value
int PIN_UV = A1;          //UV Analog value
int PIN_ULTRASONIC = A3;  //Analog distance value
int PIN_V_BATTERY = A4;   //Analog volt value
int PIN_A_BATTERY = A5;   //Analog amp value

int LEDStatus = 6;        //LED Status
int PIN_RAIN = 19;        //Rain Boolean value

int Port0 = 2;    //4 bits state
int Port1 = 3;
int Port2 = 5;

//////////////////////////////////////////////////////////
///////////                                    ///////////
///////////            SD CARD PINs            ///////////
///////////                                    ///////////
//////////////////////////////////////////////////////////
//MISO  - PIN_50   MOSI - PIN_51
//SCK   - PIN_52    CS  - PIN_53
File myFile;
char file_name[] = "DataLogs.txt";
int PIN_CHIPSELECT = 53;
bool hasSD_Card = false;

/////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////                                                       /////////////////////
/////////////////////                         SETUP                         /////////////////////
/////////////////////                                                       /////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////
int ResetPin = 7;
int rain_count = 0;
int prev_millisec;
float measurement_value = 0.2f;
void setup(){
  Serial.begin(9600);  // start serial for output
  chat.begin(4800);
  Wire.begin();        // join i2c bus (address optional for master)

  Serial.println("Master Start!");
    
  //OLED Screen
  OLED.begin(SSD1306_SWITCHCAPVCC,0x3C); // initialize with the I2C addr 0x3C (for the 128x64)
  OLED.setRotation(3);
  showLOGO();
  
  //Check Module
  ShowCheck();

  //Process
  AddData();

  pinMode(ResetPin, OUTPUT);
  pinMode(LEDStatus, OUTPUT);
  pinMode(Button1Pin, INPUT);
  pinMode(Button2Pin, INPUT);
  pinMode(Port0, INPUT);
  pinMode(Port1, INPUT);
  pinMode(Port2, INPUT);
  digitalWrite(ResetPin, LOW);  
  digitalWrite(LEDStatus, LOW);
  digitalWrite(Port0, LOW);
  digitalWrite(Port1, LOW);
  digitalWrite(Port2, LOW);
  digitalWrite(Button1Pin, LOW);
  digitalWrite(Button2Pin, LOW);
  
  attachInterrupt(digitalPinToInterrupt(PIN_RAIN), Calculate_Rain, FALLING);

  for(int i = 0; i < TEST_PACKAGE; i++){
    if(hasRTC) getRTCdata();
    checkGSM();
    showOLED();
    //Serial.print(analogRead(PIN_ULTRASONIC) * magicNumber);
    getANLdata();
    //Serial.print("\t");
    //Serial.println((float)data["analog"][1]);
    if(hasTSL) getTSLdata();
    if(hasHTU) getHTUdata();
    if(hasBME) getBMEdata();
    if(hasDHT) getDHTdata();
    data.printTo(Serial);
    Serial.println();
  }
  String output;
  data.printTo(output);
  Serial.println("\n---------------------------------SEND DATA TO CHAT---------------------------------");
  chat.print(output);
  Serial.println(output);
  showSend();
  Serial.println("-----------------------------------------------------------------------------------\n");
  
  writeSDCardFile(file_name, createDataLog());
  //readSDCardFile(file_name);

  rain_count = 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////                                                       /////////////////////
/////////////////////                         LOOPs                         /////////////////////
/////////////////////                                                       /////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////
bool flag = true;
bool canReset = true;
bool canpush = true;
int selecter = 0;
void loop(){
  data["id"] = StationID;
  data["rain"] = rain_count * measurement_value;
  digitalWrite(LEDStatus, !digitalRead(LEDStatus));
  
  if(digitalRead(Port2) && canReset){
    writeSDCardFile(file_name, createEventLog("Slave Reset!"));
    canReset = false;
    digitalWrite(LEDStatus, LOW);
    delay(1000);
    digitalWrite(ResetPin, HIGH);
    delay(5000);
  }

  if(hasRTC) getRTCdata();
  checkGSM();
  showOLED();

  if(hasRTC){
    DateTime now = RTC.now();
    int hour = now.hour();
    int minute = now.minute();
    int second = now.second();

    if(minute % 5 == 0){
      //Serial.print(analogRead(PIN_ULTRASONIC) * magicNumber);
      getANLdata();
      //Serial.print("\t");
      //Serial.println(data["analog"][1]);
      if(hasTSL) getTSLdata();
      if(hasHTU) getHTUdata();
      if(hasBME) getBMEdata();
      if(hasDHT) getDHTdata();
    }
    
    if(minute % 5 == 1 && second == 2 && flag){
      String output;
      data.printTo(output);
      Serial.println("\n---------------------------------SEND DATA TO CHAT---------------------------------");
      chat.print(output);
      Serial.println(output);
      showSend();
      Serial.println("-----------------------------------------------------------------------------------\n");
      
      writeSDCardFile(file_name, createDataLog());
      //readSDCardFile(file_name);
    
      rain_count = 0;
      
      flag = false;
    }else{
      flag = true;
    }
    
    if(hour % 12 == 11 && minute == 58 && second == 1){
      writeSDCardFile(file_name, createEventLog("Master Reset!"));
      //readSDCardFile(file_name);
      
      digitalWrite(LEDStatus, LOW);
      delay(1000);
      digitalWrite(ResetPin, HIGH);
      delay(5000);
    }
  }
  
  //delay(200);
}

void AddData(){
  //////////////////////////////////////////////////////////
  ///////////                                    ///////////
  ///////////             SET SENSOR             ///////////
  ///////////                                    ///////////
  //////////////////////////////////////////////////////////
  pinMode(PIN_UV, INPUT);
  pinMode(PIN_REF_3V3, INPUT);
  TSL2561.setGain(TSL2561_GAIN_16X);      // set 16x gain (for dim situations)
  TSL2561.setTiming(TSL2561_INTEGRATIONTIME_13MS);  // shortest integration time (bright light)

  uint16_t x = TSL2561.getLuminosity(TSL2561_VISIBLE);     
  uint32_t lum = TSL2561.getFullLuminosity();
  uint16_t ir, full;
  ir = lum >> 16;
  full = lum & 0xFFFF;

  //////////////////////////////////////////////////////////
  ///////////                                    ///////////
  ///////////              ADD JSON              ///////////
  ///////////                                    ///////////
  //////////////////////////////////////////////////////////
  data["id"] = StationID;
  if(hasRTC){
    DateTime now = RTC.now();
    JsonArray& date = data.createNestedArray("date");  
    date.add(now.day());
    date.add(now.month());
    date.add(now.year());
    JsonArray& time = data.createNestedArray("time");
    time.add(now.hour());
    time.add(now.minute());
    time.add(now.second());
    data["t_in"] = (int)RTC.getTemperature();
  }

  data["status"] = StationStatus;

  JsonArray& analog = data.createNestedArray("analog");
  analog.add(getUVdata());

  prev_value = analogRead(PIN_ULTRASONIC) * magicNumber;
  analog.add(getUltradataEWMA());
  analog.add(get_V_Battery());
  analog.add(get_A_Battery());

  if(hasTSL){
    JsonArray& tsl = data.createNestedArray("tsl");
    tsl.add(x);
    tsl.add(ir);
    tsl.add(full);
    tsl.add(full - ir);
    tsl.add(TSL2561.calculateLux(full, ir));
  }

  data["rain"] = rain_count * measurement_value;

  if(hasHTU){
    JsonArray& htu = data.createNestedArray("htu");
    htu.add((int)Humidity.readHumidity());
    htu.add((int)Humidity.readTemperature());
  }

  if(hasBME){
    JsonArray& bme = data.createNestedArray("bme");
    bme.add((int)BME280.readTemperature());
    bme.add((int)(BME280.readPressure() / 100.0F));
    bme.add((int)BME280.readAltitude(SEALEVELPRESSURE_HPA));
    bme.add((int)BME280.readHumidity());
  }

  if(hasDHT){
    JsonArray& dht = data.createNestedArray("dht");
    dht.add((int)dht12.readTemperature());
    dht.add((int)dht12.readHumidity());
  }
}

/////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////                                                       /////////////////////
/////////////////////                       GET DATUM                       /////////////////////
/////////////////////                                                       /////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////                                                       /////////////////////
/////////////////////                   I2C COMMUNICATION                   /////////////////////
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
  
  data["t_in"] = (int)RTC.getTemperature();

  data["status"] = StationStatus;
}

void getTSLdata(){
  uint16_t x = TSL2561.getLuminosity(TSL2561_VISIBLE);     
  //uint16_t x = TSL2561.getLuminosity(TSL2561_FULLSPECTRUM);
  //uint16_t x = TSL2561.getLuminosity(TSL2561_INFRARED);
  
  uint32_t lum = TSL2561.getFullLuminosity();
  uint16_t ir, full;
  ir = lum >> 16;
  full = lum & 0xFFFF;
  
  data["tsl"][0] = x;
  data["tsl"][1] = ir;
  data["tsl"][2] = full;
  data["tsl"][3] = (full - ir);
  data["tsl"][4] = TSL2561.calculateLux(full, ir);
}

void getHTUdata(){
  data["htu"][0] = (int)Humidity.readHumidity();
  data["htu"][1] = (int)Humidity.readTemperature();
}

void getBMEdata(){
  data["bme"][0] = (int)BME280.readTemperature();
  data["bme"][1] = (int)(BME280.readPressure() / 100.0F);
  data["bme"][2] = (int)BME280.readAltitude(SEALEVELPRESSURE_HPA);
  data["bme"][3] = (int)BME280.readHumidity();
}

void getDHTdata(){
  data["dht"][0] = (int)dht12.readTemperature();
  data["dht"][1] = (int)dht12.readHumidity();
}

/////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////                                                       /////////////////////
/////////////////////                 DIRECTED READ ANALOGs                 /////////////////////
/////////////////////                                                       /////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////
void getANLdata(){
  data["analog"][0] = getUVdata();
  data["analog"][1] = getUltradataEWMA();
  data["analog"][2] = get_V_Battery();
  data["analog"][3] = get_A_Battery();
}

int getUltradataMode(){
  for(int i = 0; i < VALUE_SIZE; i++){
    u_value[i] = analogRead(PIN_ULTRASONIC) * magicNumber;
    delay(delaytime);
  }
  return (int)stats.mode(u_value, VALUE_SIZE, 10.0);
}

int getUltradataEWMA(){
  float output = (getUltradataMode() * alpha) + (prev_value * (1 - alpha));
  prev_value = output;
  return (int)output;
}

float get_V_Battery(){
  int vt_temp = analogRead(PIN_V_BATTERY);
  float value = (vt_temp * (ARDUINO_WORK_VOLTAGE / 1023.0) * 5);
  char output[10];
  value = atof(dtostrf(value,3, 2, output));
  return value;
}

float get_A_Battery(){
  int at_temp = analogRead(PIN_A_BATTERY);
  float value = (at_temp * (ARDUINO_WORK_VOLTAGE / 1023.0));
  char output[10];
  value = atof(dtostrf(value,3, 2, output));
  return value;
}

float getUVdata(){
  int uvLevel = analogRead(PIN_UV);
  int refLevel = analogRead(PIN_REF_3V3);

  //Use the 3.3V power pin as a reference to get a very accurate output value from sensor
  float outputVoltage = 3.3 / refLevel * uvLevel;

  float uvIntensity = mapfloat(outputVoltage, 0.99, 2.8, 0.0, 15.0); //Convert the voltage to a UV intensity level
  if(uvIntensity < 0){
    return 0;
  }

  if(uvIntensity < 13) return uvIntensity;
  else return -1;
}

float mapfloat(float x, float in_min, float in_max, float out_min, float out_max){
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////                                                       /////////////////////
/////////////////////                       GSM START                       /////////////////////
/////////////////////                                                       /////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////

void checkGSM(){
  bool MSB = digitalRead(Port0);
  bool LSB = digitalRead(Port1);
  if(MSB || LSB){
    GSMStart = true;
    if(MSB && LSB){
      SignalQuality = "o)))"; //Stage 11
    }
    if(MSB && !LSB){
      SignalQuality = "o))";  //Stage 10
    }
    if(!MSB && LSB){
      SignalQuality = "o)";   //Stage 01
    }
  }else{
    GSMStart = false;
    SignalQuality = "o";  //Stage 00
  }
}

/////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////                                                       /////////////////////
/////////////////////                       INTERRUPT                       /////////////////////
/////////////////////                                                       /////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////
int debouncing_delay = 50;
void Calculate_Rain(){
  int current_millisec = millis();
  if(current_millisec - prev_millisec >= debouncing_delay){
    rain_count++;
    prev_millisec = current_millisec;
  }
}

/////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////                                                       /////////////////////
/////////////////////                   SPI COMMUNICATION                   /////////////////////
/////////////////////                                                       /////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////
void readSDCardFile(char file_name[]){
  myFile = SD.open(file_name);
  if(myFile){
    Serial.print("READ ");
    Serial.print(file_name);
    Serial.println(":");
    while (myFile.available()){
      Serial.write(myFile.read());
    }
    myFile.close();
  }else{
    Serial.print("CAN'T FIND ");
    Serial.println(file_name);
  }
}

void writeSDCardFile(char file_name[], String data){
  myFile = SD.open(file_name, FILE_WRITE);
  if(myFile){
    Serial.print("WRITING TO ");
    Serial.println(file_name);
    myFile.println(data);
    myFile.close();
    Serial.println("DONE.");
  } else {
    Serial.print("CAN'T FIND ");
    Serial.println(file_name);
  }
}

String createDataLog(){
  //return "DUMMY LOG DATA [1/1/2561] - [14:7:36] - Ultrasonic: 247.12 cm - UV: 0.81 mW/cm^2";
  String _id_ = data["id"].as<String>();
  String _date_ = data["date"][0].as<String>() + "-" + data["date"][1].as<String>() + "-" + data["date"][2].as<String>();
  String _time_ = data["time"][0].as<String>() + ":" + data["time"][1].as<String>() + ":" + data["time"][2].as<String>();
  String _data_ = "ULTRA: " + data["analog"][1].as<String>() + " cm.";
  return _id_ + "\t" + _date_ + "\t" + _time_ + "   \t" + _data_;
}

String createEventLog(String event){
  String _id_ = data["id"].as<String>();
  String _date_ = data["date"][0].as<String>() + "-" + data["date"][1].as<String>() + "-" + data["date"][2].as<String>();
  String _time_ = data["time"][0].as<String>() + ":" + data["time"][1].as<String>() + ":" + data["time"][2].as<String>();
  return _id_ + "\t" + _date_ + "\t" + _time_ + "   \t" + event;
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
  if(digitalRead(Button1Pin) && canpush){
    selecter++;
    selecter %= 4;
    canpush = false;
  }else{
    canpush = true;
  }
  
  switch(selecter){
    case 0: 
      showOLED_Screen1();
      break;
    case 1: 
      showOLED_Screen4();
      break;
    case 2: 
      showOLED_Screen3();
      break;
    case 3: 
      showOLED_Screen2();
      break;
  }
}

void showOLED_Screen1(){
  OLED.clearDisplay(); 
  OLED.setTextColor(WHITE,BLACK);   //Text is white ,background is black
  OLED.setCursor(0, 0);
  OLED.setTextSize(2);
  OLED.print("PYF");
  OLED.setTextSize(1);
  OLED.setCursor(33, 8);
  OLED.println("lood");
  OLED.println("ID: " + data["id"].as<String>());
  OLED.println("STATUS:" + data["status"].as<String>());
  OLED.println();
  OLED.println("DATE:");  OLED.println(data["date"][0].as<String>() + "-" + data["date"][1].as<String>() + "-" + data["date"][2].as<String>());
  OLED.println();
  OLED.println("TIME:");  OLED.println(data["time"][0].as<String>() + ":" + data["time"][1].as<String>() + ":" + data["time"][2].as<String>());
  OLED.println();
  OLED.println("ULTRA:"); OLED.print((int)data["analog"][1]); OLED.println(" cm");
  OLED.println();
  OLED.println("RAIN: "); OLED.print((float)data["rain"]); OLED.println(" mm");
  OLED.display();
}

void showOLED_Screen2(){
  OLED.clearDisplay(); 
  OLED.setTextColor(WHITE,BLACK);   //Text is white ,background is black
  OLED.setCursor(0, 0);
  OLED.setTextSize(2);
  OLED.print("PYF");
  OLED.setTextSize(1);
  OLED.setCursor(33, 8);
  OLED.println("lood");
  OLED.println();
  OLED.println("BATTERY");
  OLED.println();
  OLED.println("VOLTAGE:"); OLED.print((float)data["analog"][2]); OLED.println(" V");
  OLED.println();
  OLED.println("CURRENT:"); OLED.print((float)data["analog"][3]); OLED.println(" A");
  OLED.println();
  OLED.println("Signal:"); OLED.print(SignalQuality);
  OLED.display();
}

void showOLED_Screen3(){
  OLED.clearDisplay(); 
  OLED.setTextColor(WHITE,BLACK);   //Text is white ,background is black
  OLED.setCursor(0, 0);
  OLED.setTextSize(2);
  OLED.print("PYF");
  OLED.setTextSize(1);
  OLED.setCursor(33, 8);
  OLED.println("lood");
  OLED.println();
  OLED.println("LIGHT");
  OLED.println();
  if(hasUV){
    OLED.println("UV:"); OLED.print((float)data["analog"][0]); OLED.println(" ");
  }
  if(hasTSL){
    OLED.println();
    /*OLED.println("X:"); OLED.print((float)data["tsl"][0]); OLED.println(" ");
    OLED.println();*/
    OLED.println("IR:"); OLED.print((float)data["tsl"][1]); OLED.println(" ");
    OLED.println();
    OLED.println("Full:"); OLED.print((float)data["tsl"][2]); OLED.println(" ");
    OLED.println();
    /*OLED.println("Visible:"); OLED.print((float)data["tsl"][3]); OLED.println(" ");
    OLED.println();*/
    OLED.println("LUX:"); OLED.print((float)data["tsl"][4]); OLED.println(" ");
  }
  OLED.display();
}

void showOLED_Screen4(){
  OLED.clearDisplay(); 
  OLED.setTextColor(WHITE,BLACK);   //Text is white ,background is black
  OLED.setCursor(0, 0);
  OLED.setTextSize(2);
  OLED.print("PYF");
  OLED.setTextSize(1);
  OLED.setCursor(33, 8);
  OLED.println("lood");
  OLED.println();
  OLED.println("WEATHER");
  if(hasRTC){
    OLED.println();
    OLED.println("T_in:"); OLED.print((int)data["t_in"]); OLED.println(" C");
  }
  if(hasHTU){
    OLED.println();
    OLED.println("T_out:"); OLED.print((int)data["htu"][1]); OLED.println(" C");
    OLED.println();
    OLED.println("Humidity:"); OLED.print((int)data["htu"][0]); OLED.println(" %");
  }
  if(hasBME){
    OLED.println();
    OLED.println("T_out:"); OLED.print((int)data["bme"][0]); OLED.println(" C");
    OLED.println();
    OLED.println("Pressure:"); OLED.print((int)data["bme"][1]); OLED.println(" hPa");
    OLED.println();
    /*OLED.println("Altitude:"); OLED.print((float)data["bme"][2]); OLED.println(" m");
    OLED.println();*/
    OLED.println("Humidity:"); OLED.print((int)data["bme"][3]); OLED.println(" %");
  }
  if(hasDHT){
    OLED.println();
    OLED.println("T_out:"); OLED.print((int)data["dht"][0]); OLED.println(" C");
    OLED.println();
    OLED.println("Humidity:"); OLED.print((int)data["dht"][1]); OLED.println(" %");
  }
  OLED.display();
}

void showSend(){
  OLED.clearDisplay(); 
  OLED.setTextColor(WHITE,BLACK);   //Text is white ,background is black
  OLED.setCursor(0, 0);
  OLED.setTextSize(2);
  OLED.println("OASYS");
  OLED.setTextSize(1);
  OLED.println("send data");
  OLED.println();
  OLED.println("ULTRA:"); OLED.print((int)data["analog"][1]); OLED.println(" cm");
  OLED.display();
  delay(2000);
  OLED.clearDisplay();
}

void ShowCheck(){
  Serial.println("Check Modules");
  
  OLED.clearDisplay();
  OLED.setTextColor(WHITE,BLACK);   //Text is white ,background is black
  OLED.setCursor(0, 0);
  OLED.setTextSize(2);
  OLED.println("OASYS");
  OLED.setTextSize(1);
  OLED.println("module OK?");
  OLED.display();
  OLED.println();
  
  //Real Time Clock
  RTC.begin();
  /*DateTime now = RTC.now();
  now.day();*/
  if(RTC.now().day() > 31){
    Serial.println("RTC is NOT running!");
    OLED.println("No RTC?");
    OLED.display();
    hasRTC = false;
    // following line sets the RTC to the date & time this sketch was compiled
    RTC.adjust(DateTime(__DATE__, __TIME__));
  }else{
    Serial.println("Found RealTime Clock");
    OLED.println("RTC OK!");
    OLED.display();
    hasRTC = true;
  }
  
  delay(500);
  
  //UV Sensor
  if(getUVdata() == -1){
    Serial.println("No UV sensor?");
    OLED.println("No UV?");
    OLED.display();
    hasUV = false;
  }else{
    Serial.println("Found UV Sensor");
    OLED.println("UV OK!");
    OLED.display();
    hasUV = true;
  }
  
  delay(500);
  
  //TSL Sensor
  TSL2561.begin();
  if(TSL2561.getFullLuminosity() >> 16 == 65535){
    Serial.println("No TSL sensor?");
    OLED.println("No TSL?");
    OLED.display();
    hasTSL = false;
  }else{
    Serial.println("Found TSL Sensor");
    OLED.println("TSL OK!");
    OLED.display();
    hasTSL = true;
  }

  delay(500);

  //HTU Sensor
  Humidity.begin();
  if(Humidity.readHumidity() == 998){
    Serial.println("No Humidity&Temperature sensor?");
    OLED.println("No HTU?");
    OLED.display();
    hasHTU = false;
  }else{
    Serial.println("Found Humidity&Temperature Sensor");
    OLED.println("HTU OK!");
    OLED.display();
    hasHTU = true;
  }

  delay(500);

  //BME Sensor
  if(!BME280.begin()){
    Serial.println("No BME?");
    OLED.println("No BME?");
    OLED.display();
    hasBME = false;
  }else{
    Serial.println("Found BME");
    OLED.println("BME OK!");
    OLED.display();
    hasBME = true;
  }

  delay(500);

  //DHT Sensor
  dht12.begin();
  // Reading temperature or humidity takes about 250 milliseconds!
  // Read temperature as Celsius (the default)
  float t12 = dht12.readTemperature();
  // Read temperature as Fahrenheit (isFahrenheit = true)
  float f12 = dht12.readTemperature(true);
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  float h12 = dht12.readHumidity();
  if (isnan(h12) || isnan(t12) || isnan(f12)){
    Serial.println("No DHT?");
    OLED.println("No DHT?");
    OLED.display();
    hasDHT = false;
  }else{
    Serial.println("Found DHT");
    OLED.println("DHT OK!");
    OLED.display();
    hasDHT = true;
  }
  
  delay(500);

  //SD Card
  pinMode(SS, OUTPUT);
  if (!SD.begin(PIN_CHIPSELECT)){
    Serial.println("No SD Card?");
    OLED.println("No SDCard?");
    OLED.display();
    hasSD_Card = false;
  }else{
    Serial.println("Found SD Card");
    OLED.println("SDCard OK!");
    OLED.display();
    hasSD_Card = true;
  }
  
  delay(2000);
  OLED.clearDisplay();
}

