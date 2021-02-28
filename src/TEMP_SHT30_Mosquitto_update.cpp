#include <Arduino.h>
#include <WEMOS_SHT3X.h>

#include "EspMQTTClient.h" // https://github.com/plapointe6/EspMQTTClient 


#define UpdateMinutes 120
#define ProductKey "e7c917b9-4b86-4f99-8c7a-52449665d3c8"
#define Version "21.02.28.4"

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

EspMQTTClient msqttc(
  "12otb24e", 
  "Sukoro70",
  "192.168.9.100",   // MQTT Broker server ip
  "openhabian",      // Can be omitted if not needed
  "Sukoro70",        // Can be omitted if not needed
  ssid               // Client name that uniquely identify your device
);

uint64_t msqttLinkError=0;

unsigned long time_last_update=millis();
unsigned long time_now;
int update_ret=0;

void setup() {

  Serial.begin(115200);
  Serial.println();
 
  Serial.print("DeviceId: ");
  Serial.println(ssid);

  Serial.print("Temperature topic: ");
  Serial.println(temp_topic);

  Serial.print("Humidity topic: ");
  Serial.println(hum_topic);
  
  Serial.print("SW version: ");
  Serial.println(Version);

  Serial.print("Updateinterval (min): ");
  Serial.println(UpdateMinutes);

  msqttc.enableDebuggingMessages(true);

  temp_P=0;
  hum_P=0;
}

void loop() {
  msqttc.loop();

  if (msqttc.isConnected()){
    if(sht30.get()==0){
      temp = sht30.cTemp;
      hum = sht30.humidity;
      if (absf(temp-temp_P)>0.2 || absf(hum-hum_P)>5) {
        Serial.print("MQTT  Temperature in Celsius : ");
        Serial.println(temp);
        Serial.print("MQTT  Relative Humidity : ");
        Serial.println(hum);
        Serial.println();
        msqttc.publish(temp_topic, String(temp));
        msqttc.publish(hum_topic, String(hum));
        temp_P=temp;
        hum_P=hum;
        time_now=millis();
        Serial.print("-> Time up sec: ");
        Serial.println(time_now/1000);
      }
    }
    else
    {
      Serial.println("Sensor Reading Error!");
    }
  }
  else {
    msqttLinkError++;
    if (msqttLinkError==500000) {
      Serial.println("MSQTT Not Connected Error!");
      msqttLinkError=0;
    }  
  }

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
