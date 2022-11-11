#include "Config.h"
#include <NeoPixel.h>
#include <NeoPatterns.hpp>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266httpUpdate.h>
#include <PubSubClient.h>
#include <TimeLib.h>
#include "BarLight.h"

void processMotionSensor(unsigned long milliseconds) {
  if (milliseconds >= lastTime + SENSORDELAY)
  {
    motionValue = digitalRead(SENSOR_PIN);
    if (motionValue == HIGH) {
      Serial.println("Motion detected");
      if (strip.ActivePattern == PATTERN_DELAY)
        strip.Delay(HOLDTIME);
      else
        strip.Fade(COLOR32_BLACK, COLOR32_WHITE, 64, 20);
      lastTime = milliseconds;
    }
  }
}

void animComplete(NeoPatterns *light) {
  Serial.println("Anim complete");
  if (light->ActivePattern == PATTERN_FADE && light->Color1 == COLOR32_BLACK) {
    Serial.println("Switching to Delay");
    light->Delay(HOLDTIME);
  } else if (light->ActivePattern == PATTERN_DELAY) {
    Serial.println("Switching to heartbeat");
    light->RainbowCycle(1000);
    //light->Heartbeat(COLOR32_WHITE, 40, 5);
  } else if (light->ActivePattern == PATTERN_HEARTBEAT) {
    Serial.println("Switching to fade");
    light->Fade(COLOR32_WHITE, COLOR32_BLACK, 64, 20);
  }  else {
    Serial.println("Switching off");
    SetValue(0,0,0);
  }

}

bool validateWiFi(unsigned long milliseconds) {
  // Update WiFi status. Take care of rollover
  if (milliseconds >= lastTimeClock + 1000 || milliseconds < lastTimeClock) {
    if (wifiMulti.run() != WL_CONNECTED) {
      Serial.println("Disconnected");
      mqttLog("Connecting");
      connectedOnce = false;
    } else {
      if (!connectedOnce) {
        Serial.print("Connected late to ");
        Serial.println(WiFi.SSID());
        mqttLog("");
      }
      connectedOnce = true;
    }

    lastTimeClock = milliseconds;
  }

  return connectedOnce;
}

void validateMqtt(unsigned long milliseconds) {
  if (!mqttClient.connected()) {
    if (milliseconds - lastReconnectAttempt > 5000 || lastReconnectAttempt == 0 || milliseconds < lastReconnectAttempt) {
      Serial.println("MQTT not connected");
      lastReconnectAttempt = milliseconds;
      Serial.println("MQTT reconnecting");
      // Attempt to reconnect
      if (mqttReconnect()) {
        Serial.println("MQTT reconnected");
      }
    }

    if (milliseconds - lastReconnectAttempt > 60000) {
      Serial.println("MQTT disconnecting WiFi");
      WiFi.disconnect();
      delay(500);
    }
  } else {
    mqttClient.loop();
  }
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  Serial.printf("Inside mqtt callback: %s\n", topic);
  Serial.println(length);

  String topicString = (char*)topic;
  topicString = topicString.substring(topicString.lastIndexOf('/')+1);

  Serial.print("Action: ");
  Serial.println(topicString);

  if (topicString == "control") {
    String message = (char*)payload;
    message = message.substring(0, length);
    Serial.println(message);
    mqttLog(message.c_str());
  }
  if (topicString == "update") {
    WiFiClient updateWiFiClient;
    t_httpUpdate_return ret = ESPhttpUpdate.update(updateWiFiClient, UPDATE_URL);
    switch (ret) {
      case HTTP_UPDATE_FAILED:
        Serial.printf("HTTP_UPDATE_FAILD Error (%d): %s\n", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
        break;

      case HTTP_UPDATE_NO_UPDATES:
        Serial.println("HTTP_UPDATE_NO_UPDATES");
        break;

      case HTTP_UPDATE_OK:
        Serial.println("HTTP_UPDATE_OK");
        break;
    }
  }
}

void mqttLog(const char* status) {
  if (mqttClient.connected()) {
    mqttClient.publish(MQTT_CHANNEL_LOG, status, false);
  }
}

void mqttLog(String status)
{
  char buf[256];
  status.toCharArray(buf, 256);
  mqttLog(buf);
}

void mqttPublish(const char* status) {
  if (mqttClient.connected()) {
    mqttClient.publish(MQTT_CHANNEL_PUB, status, true);
  }
}

boolean mqttReconnect() {
  char buf[100];
  mqttClientId.toCharArray(buf, 100);
  if (mqttClient.connect(buf, MQTT_USER, MQTT_PASSWORD)) {
    mqttClient.subscribe(MQTT_CHANNEL_SUB);
  }

  return mqttClient.connected();
}

String generateMqttClientId() {
  char buffer[4];
  uint8_t macAddr[6];
  WiFi.macAddress(macAddr);
  sprintf(buffer, "%02x%02x", macAddr[4], macAddr[5]);
  return "BarLight" + String(buffer);
}

void SetValue(int r, int g, int b) {
  for (int i=0; i < NUMLEDS; i++)
    strip.setPixelColor(i, r, g, b);
  strip.show();
}

void update_started() {
  Serial.println("CALLBACK:  HTTP update process started");
  SetValue(0,0,0);
}

void update_finished() {
  Serial.println("CALLBACK:  HTTP update process finished");
  SetValue(0,50,0);
  mqttLog("Update Finished");
}

void update_progress(int progress, int total) {
  Serial.printf("CALLBACK:  HTTP update process at %d of %d bytes...\n", progress, total);
  mqttLog("Update progress");
  for (int i = 0; i < (NUMLEDS * progress / total); i++)
    strip.setPixelColor(i, 0, 0, 50);
  strip.show();

}

void update_error(int err) {
  Serial.printf("CALLBACK:  HTTP update fatal error code %d\n", err);
  mqttLog("Update error");
  mqttLog(String(err));
  SetValue(50,0,0);
}

void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println();
  Serial.println();
  Serial.println('Hi');

  for (int i=0; i < wifiCount; i++) {
    wifiMulti.addAP(ssids[i], passs[i]);
  }

  Serial.println("Connecting");
  if (wifiMulti.run() == WL_CONNECTED) {
    Serial.print("Connected: ");
    Serial.println(WiFi.localIP());
    mqttLog("Connected");
    connectedOnce = true;
  }


  mqttClientId = generateMqttClientId();

  ESPhttpUpdate.setLedPin(LED_BUILTIN, LOW);
  ESPhttpUpdate.onStart(update_started);
  ESPhttpUpdate.onEnd(update_finished);
  ESPhttpUpdate.onProgress(update_progress);
  ESPhttpUpdate.onError(update_error);

  pinMode(SENSOR_PIN, INPUT);
  strip.begin();
  SetValue(0,0,0);

}

void loop() {
  // Get the time at the start of this loop
  unsigned long milliseconds = millis();

  processMotionSensor(milliseconds);
  strip.update();
  if (validateWiFi(milliseconds))
    validateMqtt(milliseconds);
}
