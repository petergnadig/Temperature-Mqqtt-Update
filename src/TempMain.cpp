#include <Arduino.h>
#include <WEMOS_SHT3X.h>
#include <ArduinoJson.h>

#include "EspMQTTClient.h" // https://github.com/plapointe6/EspMQTTClient 

//#include <Wire.h>               // Only needed for Arduino 1.6.5 and earlier
#include "SSD1306Wire.h"        // legacy: #include "SSD1306.h"
SSD1306Wire display(0x3c, SDA, SCL, GEOMETRY_64_48); 
String dspstr;

#define UpdateMinutes 120
#define ProductKey "e7c917b9-4b86-4f99-8c7a-52449665d3c8"
#define Version "22.12.23.7"

#include "OtadriveUpdate.h"

void onConnectionEstablished();
float absf(float i);

SHT3X sht30(0x45);

float temp, hum, temp_P, hum_P;
char ssid[23];

uint64_t chipid = ESP.getChipId();
uint16_t chip = (uint16_t)(chipid >> 32);
int ch = snprintf(ssid, 23, "ESP8266-%04X%08X", chip, (uint32_t)chipid);

String temp_topic = String("sensor/") + ssid + String("/temperature");
String hum_topic = String("sensor/") + ssid + String("/humidity");
String json_topic = String("sensor/") + ssid + String("/json");

EspMQTTClient msqttc(
  "12otb24e", 
  "Sukoro70",
  "mosquitto.lan",   // MQTT Broker server ip
  "mqttuser",      // Can be omitted if not needed
  "pass",        // Can be omitted if not needed
  ssid               // Client name that uniquely identify your device
);

uint64_t msqttLinkError=0;

unsigned long time_last_update=millis();
unsigned long time_last_measure=0;
unsigned long time_now;
int update_ret=0;

StaticJsonDocument<200> doc;
char jsonoutput[200];

void setup() {

  Serial.begin(115200);
  Serial.println();
 
  Serial.print("DeviceId: ");
  Serial.println(ssid);
  
  display.init();
  display.flipScreenVertically();
  
  display.clear();
  display.setTextAlignment(TEXT_ALIGN_CENTER_BOTH);
  delay(200);

  display.setFont(ArialMT_Plain_10);
  dspstr=String(ssid);
  display.drawString(32, 24, dspstr);
  display.display();

  Serial.print("Temperature topic: ");
  Serial.println(temp_topic);

  Serial.print("Humidity topic: ");
  Serial.println(hum_topic);
  
  Serial.print("SW version: ");
  Serial.println(Version);

  Serial.print("Updateinterval (min): ");
  Serial.println(UpdateMinutes);

  msqttc.enableDebuggingMessages(false);

  temp_P=0;
  hum_P=0;


  doc["sensor"] = "TempHum";
  doc["temp"] = 0.0;
  doc["hum"] =0.0;

}

void loop() {
  msqttc.loop();

  if(sht30.get()==0){
    temp = sht30.cTemp;
    hum = sht30.humidity;
  }
  else
  {
      Serial.println("Sensor Reading Error!");
  }
 
  if (msqttc.isConnected()){
    time_now=millis();
    if (
        ((absf(temp-temp_P)>0.2 || absf(hum-hum_P)>0.2 ) && time_now-time_last_measure>1000) || 
        time_now-time_last_measure>5000
       ) 
      {
      Serial.println();
      Serial.print("MQTT  Temperature in Celsius : ");
      Serial.println(temp);
      Serial.print("MQTT  Relative Humidity : ");
      Serial.println(hum);
      msqttc.publish(temp_topic, String(temp));
      msqttc.publish(hum_topic, String(hum));
      doc["temp"] = temp;
      doc["hum"] =hum;
      serializeJson(doc, jsonoutput);
      msqttc.publish(json_topic, jsonoutput);
      temp_P=temp;
      hum_P=hum;
      time_last_measure=time_now;
      Serial.print("-> Time up sec: ");
      Serial.println(time_now/1000);
      }
    }
  else {
    msqttLinkError++;
    if (msqttLinkError==500000) {
      Serial.println("MSQTT Not Connected Error!");
      msqttLinkError=0;
    }  
  }

  display.clear();
  display.setTextAlignment(TEXT_ALIGN_CENTER_BOTH);
  display.setFont(ArialMT_Plain_16);
  dspstr=String(temp)+" C";
  display.drawString(31, 10, dspstr);
  dspstr=String(hum)+" %";
  display.drawString(31, 28, dspstr);
  dspstr=WiFi.localIP().toString().c_str();
  display.setFont(ArialMT_Plain_10);
  display.drawString(31, 42, dspstr);
  display.display();
  delay(200);

  time_now=millis();
  if ((time_now-time_last_update)>UpdateMinutes*60*1000){
    Serial.print("----Update try----");
    Serial.println(time_now);
    update_ret= OtadriveUpdate();
    Serial.print("----Update result----");
    Serial.println(update_ret);
    time_last_update=millis();
  }
}

void onConnectionEstablished() {
  msqttc.subscribe(temp_topic, [] (const String &payload)  {
  });
  msqttc.subscribe(hum_topic, [] (const String &payload)  {
  });
}

float absf(float i) {
  if (i>0) {
    return i;
  }
  else {
    return -i;
  }
}
