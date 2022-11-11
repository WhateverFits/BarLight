#ifndef BarLightHeader
#define BarLightHeader
void mqttLog(const char* status);
void mqttPublish(const char* status);
void mqttCallback(char* topic, byte* payload, unsigned int length);
void mqttLog(String status);
void mqttLog(const char* status);
void animComplete(NeoPatterns *light);
ESP8266WiFiMulti wifiMulti;

int motionValue = 0;
unsigned long lastTime = 0;
unsigned long lastTimeClock = 0;
time_t utc, local;
int pos = 90;
int increment = 1;
bool connectedOnce = false;

NeoPatterns strip(NUMLEDS, NEOPIN, NEO_GRB + NEO_KHZ800, animComplete);

WiFiClient mqttWiFiClient;
String mqttClientId; 
long lastReconnectAttempt = 0; 

PubSubClient mqttClient(MQTT_SERVER, MQTT_PORT, mqttCallback, mqttWiFiClient);

#endif
