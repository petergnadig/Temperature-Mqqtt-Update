#if defined(ESP32)
  #include <WiFi.h>
#elif defined(ESP8266)
  #include <ESP8266WiFi.h>
#endif

struct wfcr {
  char* ssid;
  char* pass;
}
const wfcr[5] = {{"iPGXIII","1234567890"},{"12otb24f","Sukoro70" },{"12otb24e","Sukoro70"},{"SML","Sukoro70" },{"INDOTEK_GUEST","iT6uK6qT" }};
const int wfstno = 5;
unsigned long time_last_wifi=millis();
void connectwifi();
void checkwifi();
int connectfailed =0;

void connectwifi() {
  int wfcount=0;
  connectfailed++;
  WiFi.mode(WIFI_STA);
  while (wfcount<wfstno and WiFi.status() != WL_CONNECTED) {
    WiFi.begin(wfcr[wfcount].ssid, wfcr[wfcount].pass);             // Connect to the network
    Serial.print("Connecting to ");
    Serial.print(wfcr[wfcount].ssid); Serial.println(" ...");
    int i = 0;
    while (WiFi.status() != WL_CONNECTED and i<=10) { // Wait for the Wi-Fi to connect
      delay(1000);
      Serial.print(++i); Serial.print(' ');
    }
    Serial.println();
    wfcount++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println('\n');
    Serial.println("Connection established!");  
    Serial.print("IP address:\t");
    Serial.println(WiFi.localIP());   
    time_last_wifi=millis();
    connectfailed =0;
  }else{
    Serial.println('\n');
    Serial.println("NO WIFI AVAILABLE !!!");  
  }
}

void checkwifi() {
  unsigned long time_now=millis();
  if (time_now-time_last_wifi>60000) {
   time_last_wifi=millis();
   if (WiFi.status() != WL_CONNECTED) {
    Serial.println();
    Serial.println("No wifi, try to connect");
    connectwifi();
    if(connectfailed>20){
      ESP.restart();
    }
   } else {
    Serial.println();
    Serial.println("Wifi OK");
   }
 } 
}