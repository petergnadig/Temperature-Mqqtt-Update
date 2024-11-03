#ifdef __INTELLISENSE__
#pragma diag_suppress 153
#endif

#include <Arduino.h>

#include <WEMOS_SHT3X.h>
SHT3X sht30(0x45);
int16_t retsht30;

#include <ArduinoJson.h>
StaticJsonDocument<200> doc;
StaticJsonDocument<200> status;

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
#define ProductKey "a105cefa-8a00-42f7-ad6e-8dbfcb9bb3be"
#define Version "24.11.03.01"
#include "OtadriveUpdate.h"
// user:peter.gnadig@hotmail.com pass:Sukoro70

#include "gwifimulti.h"

void onConnectionEstablished();
float absf(float i);

float temp, hum, temp_P, hum_P, pres, pres_P;
char ssid[23];

uint64_t chipid = ESP.getChipId();
uint16_t chip = (uint16_t)(chipid >> 32);
int ch1 = snprintf(ssid, 23, "ESP8266-%04X%08X", chip, (uint32_t)chipid);

EspMQTTClient msqttc(
  "ot12mqtt.dyndns.org",   // MQTT Broker server ip
   1883,
  "sml",          // Can be omitted if not needed
  "sml1234",        // Can be omitted if not needed
  ssid               // Client name that uniquely identify your device
);

String temp_topic = String("sensor/") + ssid + String("/temperature");
String hum_topic = String("sensor/") + ssid + String("/humidity");
String pres_topic = String("sensor/") + ssid + String("/pressure");
String json_topic = String("sensor/") + ssid + String("/json");
char char_status_topic[200];
int ch2= snprintf(char_status_topic, 200, "%s%s%s", "state/" , ssid , "/state");
String status_topic = char_status_topic;
String status_payload;

uint64_t msqttLinkError=0;

unsigned long time_last_update=millis();
unsigned long time_last_measure=0;
unsigned long time_last_alive=0;
unsigned long time_last_display=0;
unsigned long time_now;
int update_ret=0;

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(false);
  Serial.println();
  delay(1000);
 
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

  connectwifi();

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

  msqttc.enableLastWillMessage(char_status_topic, "Offline!!! - Lastwill", false);
  msqttc.enableDebuggingMessages(false);

  temp_P=0;
  hum_P=0;
  pres_P=0;

  doc["sensor"] = "TempHum";
  doc["temp"] = 0.0;
  doc["hum"] =0.0;
  doc["pres"] = 0.0;
  doc["ver"]= Version;
  doc["IP"]=WiFi.localIP();

  status["State"] = "JustAlive";
  status["Version"] = Version;
  status["IP"]=WiFi.localIP();
}

void loop() {

  checkwifi();
  msqttc.loop();

  retsht30 = sht30.get();
  if(retsht30==0)
  {
    temp = sht30.cTemp;
    hum = sht30.humidity;
  }
  else
  {
  //  Serial.print("Sensor Reading Error :");
  //  Serial.println(retsht30);
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
    Serial.print("FAIL to read pressure! ret = ");
    Serial.println(retHP303B); 
  }

if (msqttc.isConnected()){
    if (
        ((absf(temp-temp_P)>0.9 || absf(hum-hum_P)>0.9 || absf(pres-pres_P)>5.0) && time_now-time_last_measure>1000) || 
          time_now-time_last_measure>5000
      ) 
      {
      Serial.println();
      Serial.printf("MQTT Temperature in Celsius : %.2f \n",temp);
      Serial.printf("MQTT Relative Humidity : %.2f \n",hum);
      Serial.printf("MQTT Pressure : %.2f \n",pres);
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

  time_now=millis();

  if ((time_now-time_last_display)>5000){
    display.clear();
    display.setTextAlignment(TEXT_ALIGN_CENTER_BOTH);
    display.setFont(ArialMT_Plain_10);
    dspstr=String(temp)+" C" ;
    display.drawString(31, 10, dspstr);
    dspstr=String(hum)+" %";
    display.drawString(31, 20, dspstr);
    dspstr=String(pres)+" Pa";
    display.drawString(31, 30, dspstr);
    dspstr=WiFi.localIP().toString().c_str();
    display.drawString(31, 42, dspstr);
    display.display();
    time_last_display=millis();
  }

  if ((time_now-time_last_update)>UpdateMinutes*60*1000){
    Serial.print("----Update try----");
    Serial.println(time_now);
    update_ret= OtadriveUpdate();
    Serial.print("----Update result----");
    Serial.println(update_ret);
    time_last_update=millis();
  }

  if ((time_now-time_last_alive)>5*60*1000){
    if (msqttc.isConnected()){
      status["State"] = "Alive";
      status["IP"] = WiFi.localIP().toString();
      serializeJson(status, jsonoutput);
      msqttc.publish(status_topic, jsonoutput);
      time_last_alive=millis();
    }
  }
}

void onConnectionEstablished() {
      status["State"] = "JustGotAlive";
      serializeJson(status, jsonoutput);
      msqttc.publish(status_topic, jsonoutput);
  time_last_alive=millis();
}

float absf(float i) {
  if (i>0) {
    return i;
  }
  else {
    return -i;
  }
}
