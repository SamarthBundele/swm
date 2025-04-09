#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <time.h>
#include "secrets.h"
#include <SPI.h>
#include <LoRa.h>

#define TIME_ZONE -5
#define SS    D8
#define RST   D0
#define DIO0  D1

unsigned long lastMillis = 0;

#define AWS_IOT_PUBLISH_TOPIC   "swm/pub"
#define AWS_IOT_SUBSCRIBE_TOPIC "swm/sub"

WiFiClientSecure net;
BearSSL::X509List cert(cacert);
BearSSL::X509List client_crt(client_cert);
BearSSL::PrivateKey key(privkey);

PubSubClient client(net);
time_t now;
time_t nowish = 1510592825;

void NTPConnect(void) {
  Serial.print("Setting time using SNTP");
  configTime(TIME_ZONE + TIME_OFFSET, 0, "pool.ntp.org", "time.nist.gov");
  now = time(nullptr);
  while (now < nowish) {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
  }
  Serial.println("done!");
  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);
  Serial.print("Current time: ");
  Serial.print(asctime(&timeinfo));
}

void messageReceived(char *topic, byte *payload, unsigned int length) {
  Serial.print("Received [");
  Serial.print(topic);
  Serial.print("]: ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

void connectAWS() {
  delay(3000);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.println(String("Attempting to connect to SSID: ") + String(WIFI_SSID));
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }
  NTPConnect();
  net.setTrustAnchors(&cert);
  net.setClientRSACert(&client_crt, &key);
  client.setServer(MQTT_HOST, 8883);
  client.setCallback(messageReceived);
  Serial.println("Connecting to AWS IOT");
  while (!client.connect(THINGNAME)) {
    Serial.print(".");
    delay(1000);
  }
  if (!client.connected()) {
    Serial.println("AWS IoT Timeout!");
    return;
  }
  client.subscribe(AWS_IOT_SUBSCRIBE_TOPIC);
  Serial.println("AWS IoT Connected!");
}

void publishMessage(String receivedData) {
  StaticJsonDocument<200> doc;
  doc["time"] = millis();
  doc["data"] = receivedData;
  char jsonBuffer[512];
  serializeJson(doc, jsonBuffer);
  client.publish(AWS_IOT_PUBLISH_TOPIC, jsonBuffer);
}

void setup() {
  Serial.begin(9600);
  unsigned long startTime = millis();
  while (!Serial && millis() - startTime < 5000) {
    yield();
  }
  Serial.println("LoRa Receiver");
  LoRa.setPins(SS, RST, DIO0);
  if (!LoRa.begin(433E6)) {
    Serial.println("Starting LoRa failed!");
    while (true) {
      delay(100);
      yield();
    }
  }
  Serial.println("LoRa Initializing OK!");
  connectAWS();
}

void loop() {
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    String receivedData = "";
    while (LoRa.available()) {
      char c = (char)LoRa.read();
      receivedData += c;
    }
    Serial.print("Received packet: '");
    Serial.print(receivedData);
    Serial.print("' with RSSI ");
    Serial.println(LoRa.packetRssi());
    if (!client.connected()) {
      connectAWS();
    } else {
      client.loop();
      if (millis() - lastMillis > 5000) {
        lastMillis = millis();
        publishMessage(receivedData);
      }
    }
  }
}
