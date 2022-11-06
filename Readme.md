Üvegház:
sensor/ESP8266-0000001B8D25/temperature
sensor/ESP8266-0000001B8D25/humidity


clientId-A7jE8Oy4Hl

UID: mqtt:topic:4ccbcd2729:c9b5e6af03
label: Temp07Thing
thingTypeUID: mqtt:topic
configuration: {}
bridgeUID: mqtt:broker:4ccbcd2729
location: Garage
channels:
  - id: Temp07
    channelTypeUID: mqtt:number
    label: Temp07
    description: ""
    configuration:
      stateTopic: sensor/ESP8266-000000A417EE/temperature
	  

OpenHab settings:	  
	  
UID: mqtt:topic:4ccbcd2729:5297833f32
label: Hunidity04Thing
thingTypeUID: mqtt:topic
configuration: {}
bridgeUID: mqtt:broker:4ccbcd2729
channels:
  - id: Humidity04
    channelTypeUID: mqtt:dimmer
    label: Humidity04
    description: ""
    configuration:
      stateTopic: sensor/ESP8266-000000A1C50D/humidity