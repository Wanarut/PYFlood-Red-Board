#include <SoftwareSerial.h>
#include <ArduinoJson.h>

SoftwareSerial chat(10, 11); // RX, TX
//String json;

void setup() {
  Serial.begin(9600);  // start serial for output
  chat.begin(4800);

  Serial.println("Start Slave");
}

void loop() {
  if(chat.available()) {
    String json = chat.readString();  // อ่าน Serial และนำไปเก็บในตัวแปร json +1 วินาที
    //Serial.println(json);
    
    DynamicJsonBuffer jsonBuffer;
    JsonObject& data = jsonBuffer.parseObject(json);
    data.printTo(Serial);
    Serial.println();
    delay(200);
  }
}
