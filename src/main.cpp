#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>
#include <Arduino.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <map>
#include <string>
#include <Syslog.h>
#include <WiFiUdp.h>

#include "config.h"

#ifdef MQTT_SERVER
#define MQTT_ENABLED
#endif

#ifdef SYSLOG_SERVER
#define SYSLOG_ENABLED
#endif

#ifndef MQTT_ENABLED
#ifndef SYSLOG_ENABLED
#error "Neither MQTT nor SYSLOG is configured properly"
#endif
#endif

ESP8266WebServer httpUpdateServer(80);
ESP8266HTTPUpdateServer httpUpdater;
#ifdef MQTT_ENABLED
WiFiClient espClient;
PubSubClient client(espClient);
#endif
#ifdef SYSLOG_ENABLED
WiFiUDP udpClient;
Syslog syslog(udpClient, SYSLOG_PROTO_IETF);
#endif

char line[8192];      // buffer for reading lines of log over serial
size_t lineIndex = 0; // current index in the line buffer

void mqttLoop()
{
#ifdef MQTT_ENABLED
  client.loop();
  if (client.connected())
  {
    return;
  }
  digitalWrite(LED_BUILTIN, LOW);
  int retries = 0;
  while (!client.connected())
  {
    if (retries < 150)
    {
      if (client.connect(MQTT_CLIENT_NAME, MQTT_USERNAME, MQTT_PASSWORD, MQTT_PREFIX "/availability", 0, true, "offline"))
      {
        client.publish(MQTT_PREFIX "/availability", "online", true);
      }
      else
      {
        retries++;
        delay(5000);
      }
    }
    else
    {
      ESP.restart();
    }
  }
  digitalWrite(LED_BUILTIN, HIGH);
#endif
}

void setup()
{
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  uint8_t mac[6];
  WiFi.macAddress(mac);

  WiFi.setSleepMode(WIFI_NONE_SLEEP);
  WiFi.mode(WIFI_STA);
  WiFi.hostname(HOSTNAME);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED)
    yield();

  MDNS.begin(HOSTNAME);
  MDNS.addService("http", "tcp", 80);

  httpUpdater.setup(&httpUpdateServer, OTA_UPDATE_USERNAME, OTA_UPDATE_PASSWORD);
  httpUpdateServer.begin();

#ifdef MQTT_ENABLED
  client.setServer(MQTT_SERVER, MQTT_PORT);
#endif

#ifdef SYSLOG_ENABLED
  syslog.server(SYSLOG_SERVER, SYSLOG_PORT);
  syslog.deviceHostname(HOSTNAME);
  syslog.appName(SYSLOG_APPNAME);
  syslog.defaultPriority(LOG_USER);
#endif

  Serial.begin(115200);
  Serial.swap();
  Serial.setTimeout(50);
  while (!Serial)
    ;
  delay(100);
  Serial.read(); // for whatever reason the first byte read seems to be garbage

  digitalWrite(LED_BUILTIN, HIGH);
}

void loop()
{
  mqttLoop();
  httpUpdateServer.handleClient();

  // read up to 1kB in a single loop() iteration
  for (int i = 0; i < 1024; i++)
  {
    int c = Serial.read();
    if (c < 0) // timeout from reading the serial, ie. no data (yet)
      break;

    line[lineIndex++] = c;
    if (c == '\n' || c == '\r' || c == 0 || lineIndex >= 8192)
    {
      if (lineIndex > 1)
      {
        line[lineIndex - 1] = 0; // null terminator
#ifdef MQTT_ENABLED
        client.publish(MQTT_PREFIX "/message", line, false);
#endif
#ifdef SYSLOG_ENABLED
        syslog.log(LOG_INFO, line);
#endif
      }
      lineIndex = 0;
      break;
    }
  }
}
