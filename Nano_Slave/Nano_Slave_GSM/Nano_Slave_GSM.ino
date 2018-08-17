//GSM
#include <TEE_UC20.h>
#include <call.h>
#include <sms.h>
#include <internet.h>
#include <File.h>
#include <http.h>
//Process
#include <ArduinoJson.h>
//Chat with Master
#include <SoftwareSerial.h>
SoftwareSerial chat(10, 11); // RX, TX

/////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////                                                       /////////////////////
/////////////////////                     GSM  VARIABLE                     /////////////////////
/////////////////////                                                       /////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////
INTERNET net;
UC_FILE file;
HTTP http;
CALL call;
//SIM MY by CAT internet
#define APN "internet"
#define USER ""
#define PASS ""
int SoftwareStartPin = 4;

/////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////                                                       /////////////////////
/////////////////////                     DECLARE  PINs                     /////////////////////
/////////////////////                                                       /////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////
int Port0 = 2;    //4 bits state
int Port1 = 3;
int Port2 = 5;
int LEDStatus = 6;

/////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////                                                       /////////////////////
/////////////////////                         SETUP                         /////////////////////
/////////////////////                                                       /////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////
int ResetPin = 7;
String message = "";
void setup(){  
  Serial.begin(9600);
  chat.begin(4800);

  Serial.println("Slave Start!");

  gsm.begin(&Serial1,9600);
  gsm.Event_debug = debug;
  Serial.println("Start UC20");
  gsm.PowerOn();
  digitalWrite(SoftwareStartPin, HIGH);
  while(gsm.WaitReady()){}
 
  Serial.print(F("GetOperator --> "));
  Serial.println(gsm.GetOperator());
  Serial.print(F("SignalQuality --> "));
  Serial.println(gsm.SignalQuality());

  pinMode(ResetPin, OUTPUT);
  pinMode(LEDStatus, OUTPUT);
  pinMode(Port0, OUTPUT);
  pinMode(Port1, OUTPUT);
  pinMode(Port2, OUTPUT);
  digitalWrite(ResetPin, LOW);  
  digitalWrite(LEDStatus, LOW);
  digitalWrite(Port0, LOW);
  digitalWrite(Port1, LOW);
  digitalWrite(Port2, LOW);
  checkSignal();
}

/////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////                                                       /////////////////////
/////////////////////                         LOOPs                         /////////////////////
/////////////////////                                                       /////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////
int count = 0;
int prev = 0;
void loop(){
  if(millis() - prev >= 2000){
    prev = millis();
    digitalWrite(LEDStatus, !digitalRead(LEDStatus));
  }
  
  if(call.WaitRing()){
    Serial.print("Ring: ");
    Serial.println(call.CurrentCallsMe());
    digitalWrite(Port2, HIGH);
    
    digitalWrite(LEDStatus, LOW);
    delay(1000);
    digitalWrite(ResetPin, HIGH);
    delay(5000);
  }
  
  if (gsm.available()){
    Serial.write(gsm.read());
  } 
  if (Serial.available()){
    char c = Serial.read();
    gsm.write(c);    
  }
  
  if(chat.available()){
    digitalWrite(LEDStatus, HIGH);
    String json = chat.readString();  // อ่าน Serial และนำไปเก็บในตัวแปร json +1 วินาที
    
    DynamicJsonBuffer jsonBuffer;
    JsonObject& data = jsonBuffer.parseObject(json);
    data.printTo(Serial);
    Serial.println();
    SendData_ultra(data);
    checkSignal();
    //SendData_Quality(data);
    delay(200);
  }
}

int checkSignal(){
  int SignalQuality = (int)gsm.SignalQuality();
  if(SignalQuality == 99){      //Value 99 Not known or not detectable
    digitalWrite(Port0, LOW);   //Stage 00
    digitalWrite(Port1, LOW);
  }
  if(SignalQuality <= 1){       //Value less than 1 is -111 dBm or less
    digitalWrite(Port0, LOW);   //Stage 01
    digitalWrite(Port1, HIGH);
  }
  if(SignalQuality >= 2 && SignalQuality <= 30){  //Value 2...30 is -109...-53 dBm 
    digitalWrite(Port0, HIGH);  //Stage 10
    digitalWrite(Port1, LOW);
  }
  if(SignalQuality >= 31 && SignalQuality != 99){  //Value more than 31 is -51 dBm or greater
    digitalWrite(Port0, HIGH);  //Stage 11
    digitalWrite(Port1, HIGH);
  }
  return SignalQuality;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////                                                       /////////////////////
/////////////////////                   INTERNET OF THING                   /////////////////////
/////////////////////                                                       /////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////
void SendData_ultra(JsonObject& data){
  message = "";
  Serial.println("---------------------------------SEND DATA ULTRASONIC---------------------------------");
  //String URL = "http://www.crflood.com/crflood/get_data/py.php?";
  String URL = "http://www.crflood.com/crflood/get_data/py-bank.php?";  //Test Data Base
  String field1 = "id=" + data["id"].as<String>();
  String field2 = "&u=" + data["analog"][1].as<String>();
  String field3 = "&r=" + data["rain"].as<String>();
  String field4 = "&to=" + data["bme"][0].as<String>();
  String field5 = "&h=" + data["bme"][3].as<String>();
  String field6 = "&p=" + data["bme"][1].as<String>();
  String field7 = "&rs=" + (String)checkSignal();
  String field8 = "&ti=" + data["t_in"].as<String>();
  String field9 = "&v=" + data["analog"][2].as<String>();
  String field10 = "&i=" + data["analog"][3].as<String>();
  String SendData_string = URL + field1 + field2 + field3 + field4 + field5 + field6 + field7 + field8 + field9 + field10;

  Serial.println(SendData_string);

  Serial.println(F("Disconnect net"));
  net.DisConnect();
  Serial.println(F("Set APN and Password"));
  net.Configure(APN, USER, PASS);
  Serial.println(F("Connect net"));
  net.Connect();
  Serial.println(F("Show My IP"));
  Serial.println(net.GetIP());
  Serial.println(F("Start HTTP"));
  http.begin(1);
  Serial.println(F("Send HTTP GET"));
  http.url(SendData_string);
  Serial.println(http.get());

  Serial.println(F("Clear data in RAM"));
  file.Delete(RAM, "*");
  Serial.println(F("Save HTTP Response To RAM"));
  http.SaveResponseToMemory(RAM, "web.hml");
  Serial.println(F("Read data in RAM"));
  read_message(RAM,"web.hml");

  Serial.println(F("Disconnect net"));
  net.DisConnect();
}

void SendData_Quality(JsonObject& data){
  message = "";
  Serial.println("---------------------------------SEND DATA QUALITY---------------------------------");
  String URL = "http://www.thaiwaterqq.com/api-qq/send_db_quality.php?";
  String field1 = "&StationID=" + data["id"].as<String>();
  String field2 = "&D_O=" + (String)("%.2f",          (float)data["time"][1]);
  String field3 = "&pH=" + (String)("%.2f",           (float)data["time"][2]);
  String field4 = "&Conductivity=" + (String)("%.2f", (float)data["hum&temp"][0]);
  String field5 = "&temp=" + (String)("%.2f",         (float)data["hum&temp"][1]);
  String field6 = "&status=" + data["status"].as<String>();
  String field7 = "&hr=" + (String)("%.2f",           (float)data["time"][0]);
  String field8 = "&Ti=" + (String)("%.2f",           (float)data["temp_in"]);
  String field9 = "&uv=" + (String)("%.3f",           (float)data["analog"][1]);
  String field10 = "&lum=" + (String)("%.2f",         (float)data["light"][0]);
  String field11 = "&power=" + (String)("%.2f",       (float)data["analog"][4]);
  
  String SendData_string = URL + field1 + field2 + field3 + field4 + field5 + field6 + field7 + field8 + field9 + field10 + field11;

  Serial.println(SendData_string);

  Serial.println(F("Disconnect net"));
  net.DisConnect();
  Serial.println(F("Set APN and Password"));
  net.Configure(APN, USER, PASS);
  Serial.println(F("Connect net"));
  net.Connect();
  Serial.println(F("Show My IP"));
  Serial.println(net.GetIP());
  Serial.println(F("Start HTTP"));
  http.begin(1);
  delay(1000);
  Serial.println(F("Send HTTP GET"));
  http.url(SendData_string);
  Serial.println(http.get());
 
  Serial.println(F("Clear data in RAM"));
  file.Delete(RAM,"*");
  Serial.println(F("Save HTTP Response To RAM"));
  http.SaveResponseToMemory(RAM,"web.hml");
  Serial.println(F("Read data in RAM"));
  read_file(RAM,"web.hml");
  
  Serial.println(F("Disconnect net"));
  net.DisConnect();
}

/////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////                                                       /////////////////////
/////////////////////                         DEBUG                         /////////////////////
/////////////////////                                                       /////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////
void debug(String data){
  Serial.println(data);
}

void data_out(char data){
  Serial.write(data);
}

void read_file(String pattern,String file_name){
  file.DataOutput =  data_out;
  file.ReadFile(pattern,file_name);
}
void set_message(char data){
  message = String(message + data);
  Serial.write(data);
}
void read_message(String pattern,String file_name){
  file.DataOutput =  set_message;
  file.ReadFile(pattern,file_name);
}
