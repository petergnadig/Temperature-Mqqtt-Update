#include <Arduino.h>
#include <WEMOS_SHT3X.h>
SHT3X sht30(0x45);
int16_t retsht30;

#include <ArduinoJson.h>
StaticJsonDocument<200> doc;
char jsonoutput[200];

#include <LOLIN_HP303B.h>  // https://github.com/wemos/LOLIN_HP303B_Library 
LOLIN_HP303B HP303BPressureSensor;
int16_t oversampling = 7;
int16_t retHP303B;
int32_t temperature;
int32_t pressure;

#include "EspMQTTClient.h" // https://github.com/plapointe6/EspMQTTClient 

#include "SSD1306Wire.h"        // legacy: #include "SSD1306.h"
SSD1306Wire display(0x3c, SDA, SCL, GEOMETRY_64_48); 
String dspstr;

#define UpdateMinutes 120
#define ProductKey "e7c917b9-4b86-4f99-8c7a-52449665d3c8"
#define Version "22.12.26.8"
#include "OtadriveUpdate.h"

void onConnectionEstablished();
float absf(float i);

float temp, hum, temp_P, hum_P, pres, pres_P;
char ssid[23];

uint64_t chipid = ESP.getChipId();
uint16_t chip = (uint16_t)(chipid >> 32);
int ch = snprintf(ssid, 23, "ESP8266-%04X%08X", chip, (uint32_t)chipid);

EspMQTTClient msqttc(
  "12otb24e", 
  "Sukoro70",
  "mosquitto.lan",   // MQTT Broker server ip
  "mqttuser",      // Can be omitted if not needed
  "pass",        // Can be omitted if not needed
  ssid               // Client name that uniquely identify your device
);

String temp_topic = String("sensor/") + ssid + String("/temperature");
String hum_topic = String("sensor/") + ssid + String("/humidity");
String pres_topic = String("sensor/") + ssid + String("/pressure");
String json_topic = String("sensor/") + ssid + String("/json");

uint64_t msqttLinkError=0;

unsigned long time_last_update=millis();
unsigned long time_last_measure=0;
unsigned long time_now;
int update_ret=0;

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

  HP303BPressureSensor.begin();

  Serial.print("Temperature topic: ");
  Serial.println(temp_topic);

  Serial.print("Humidity topic: ");
  Serial.println(hum_topic);
  
  Serial.print("Pressure topic: ");
  Serial.println(pres_topic);

  Serial.print("SW version: ");
  Serial.println(Version);

  Serial.print("Updateinterval (min): ");
  Serial.println(UpdateMinutes);

  msqttc.enableDebuggingMessages(false);

  temp_P=0;
  hum_P=0;
  pres_P=0;

  doc["sensor"] = "TempHum";
  doc["temp"] = 0.0;
  doc["hum"] =0.0;
  doc["pres"] = 0.0;

}

void loop() {
  msqttc.loop();
  time_now=millis();

  retsht30 = sht30.get();
  if(retsht30==0)
  {
    temp = sht30.cTemp;
    hum = sht30.humidity;
  }
  else
  {
    Serial.print("Sensor Reading Error :");
    Serial.println(retsht30);
  }
 
  retHP303B = HP303BPressureSensor.measurePressureOnce(pressure, oversampling);
  if (retHP303B == 0)
  {
    pres=pressure;
  }
  else
  {
    //Something went wrong. Look at the library code for more information about return codes
    Serial.print("FAIL to reade pressure! ret = ");
    Serial.println(retHP303B); 
  }
  retHP303B = HP303BPressureSensor.measureTempOnce(temperature, oversampling);
  if (retHP303B == 0)
  {
    if (retsht30!=0) {temp=temperature;}
  }
  else
  {
    //Something went wrong. Look at the library code for more information about return codes
    Serial.print("FAIL to reade pressure! ret = ");
    Serial.println(retHP303B); 
  }

if (msqttc.isConnected()){
    if (
        ((absf(temp-temp_P)>0.2 || absf(hum-hum_P)>0.2 || absf(pres-pres_P)>1.0) 
          && time_now-time_last_measure>1000) || 
          time_now-time_last_measure>5000
      ) 
      {
      Serial.println();
      Serial.print("MQTT  Temperature in Celsius : ");
      Serial.println(temp);
      Serial.print("MQTT  Relative Humidity : ");
      Serial.println(hum);
      Serial.print("MQTT  Pressure : ");
      Serial.println(pres);
      msqttc.publish(temp_topic, String(temp));
      msqttc.publish(hum_topic, String(hum));
      msqttc.publish(pres_topic, String(pres));
      doc["temp"] = temp;
      doc["hum"] =hum;
      doc["pres"]= pres;
      serializeJson(doc, jsonoutput);
      msqttc.publish(json_topic, jsonoutput);
      temp_P=temp;
      hum_P=hum;
      pres_P=pres;
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
  display.setFont(ArialMT_Plain_10);
  dspstr=String(temp)+" C";
  display.drawString(31, 10, dspstr);
  dspstr=String(hum)+" %";
  display.drawString(31, 20, dspstr);
  dspstr=String(pres)+" Pa";
  display.drawString(31, 30, dspstr);
  dspstr=WiFi.localIP().toString().c_str();
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

}

float absf(float i) {
  if (i>0) {
    return i;
  }
  else {
    return -i;
  }
}