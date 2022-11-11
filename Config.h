#ifndef _CONFIG
#define _CONFIG

#define SENSOR_PIN D1
#define BUTTON_PIN D3
#define LEDPIN D0
#define NEOPIN D2
#define DNSNAME "barlight"
#define MQTT_SERVER "pi4"
#define MQTT_PORT 1883
#define MQTT_CHANNEL_PUB "home/" DNSNAME "/state"
#define MQTT_CHANNEL_SUB "home/" DNSNAME "/#"
#define MQTT_CHANNEL_LOG "home/" DNSNAME "/log"
#define MQTT_USER "clockuser"
#define MQTT_PASSWORD "clockuser"
#define UPDATE_URL "http://pi4/cgi-bin/test.rb"
#define MQTT_MAX_PACKET_SIZE 20000
#define NUMLEDS 20
#define HOLDTIME 90000
#define SENSORDELAY 10000


const char* ssids[] = {"WiFi"};
const char* passs[] = {"Here"};
const int wifiCount = 1;

#endif
