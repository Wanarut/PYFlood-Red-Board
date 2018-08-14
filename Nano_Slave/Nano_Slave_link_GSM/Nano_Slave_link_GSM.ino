#include "TEE_UC20.h"
#include "SoftwareSerial.h"
#include <AltSoftSerial.h>
#include "call.h"
#include "sms.h"
#include "internet.h"
#include "File.h"
#include "http.h"

SoftwareSerial chat(10, 11); // RX, TX
AltSoftSerial mySerial;

INTERNET net;
UC_FILE file;
HTTP http;
CALL call;

//SIM MY by CAT internet
#define APN "internet"
#define USER ""
#define PASS ""

String message = "";

/////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////                                                       /////////////////////
/////////////////////                         SETUP                         /////////////////////
/////////////////////                                                       /////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////
int SoftwareStartPin = 4;
int ResetPin = 7;
void setup(){
  Serial.begin(9600);
  chat.begin(4800);  

  gsm.begin(&mySerial,9600);
  gsm.Event_debug = debug;
  Serial.println("Start UC20");
  gsm.PowerOn(); 
  digitalWrite(SoftwareStartPin, HIGH);
  pinMode(ResetPin, OUTPUT);
  while(gsm.WaitReady()){}
 
  Serial.print("GetOperator --> ");
  Serial.println(gsm.GetOperator());
  Serial.print("SignalQuality --> ");
  Serial.println(gsm.SignalQuality());
}

/////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////                                                       /////////////////////
/////////////////////                         LOOPs                         /////////////////////
/////////////////////                                                       /////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////
void loop(){
  if(call.WaitRing()){
    Serial.print("Ring");
    Serial.println(call.CurrentCallsMe());
    delay(1000);
    digitalWrite(ResetPin, HIGH);
  }
  
  if (gsm.available()){
    Serial.write(gsm.read());
  } 
  if (Serial.available()){
    char c = Serial.read();
    gsm.write(c);    
  }
  
  if(chat.available()) {
    Serial.println("CHAT AVAILABLE");
    String url = chat.readString();
    SendData_Quality(url);
  }
}

/////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////                                                       /////////////////////
/////////////////////                   INTERNET OF THING                   /////////////////////
/////////////////////                                                       /////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////
void SendData_Quality(String SendData_string) 
{
  message = "";
  Serial.println("--SEND DATA QUALITY--");
  Serial.println(SendData_string);

  Serial.println("Disconnect net");
  net.DisConnect();
  Serial.println("Set APN and Password");
  net.Configure(APN, USER, PASS);
  Serial.println("Connect net");
  net.Connect();
  Serial.println("Show My IP");
  Serial.println(net.GetIP());
  Serial.println("Start HTTP");
  http.begin(1);
  Serial.println("Send HTTP GET");
  http.url(SendData_string);
  Serial.println(http.get());
 
  Serial.println("Clear data in RAM");
  file.Delete(RAM,"*");
  Serial.println("Save HTTP Response To RAM");
  http.SaveResponseToMemory(RAM,"web.hml");
  Serial.println("Read data in RAM");
  read_file(RAM,"web.hml");
  Serial.println();
  Serial.println("Disconnect net");
  net.DisConnect();
}

/////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////                                                       /////////////////////
/////////////////////                         DEBUG                         /////////////////////
/////////////////////                                                       /////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////
void debug(String data)
{
  Serial.println(data);
}

void data_out(char data)
{
  Serial.write(data);
}

void read_file(String pattern,String file_name)
{
  file.DataOutput =  data_out;
  file.ReadFile(pattern,file_name);
}
void set_message(char data)
{
  message = String(message + data);
  Serial.write(data);
}
void read_message(String pattern,String file_name)
{
  file.DataOutput =  set_message;
  file.ReadFile(pattern,file_name);
}
