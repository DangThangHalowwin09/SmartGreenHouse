#include <iostream>
#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <ArduinoJson.h>
#include <stdlib.h>

const char* ssid = "Dangthang09";
const char* pass = "123456789";
const char* host = "broker.hivemq.com";
const int port = 1883;
const char* username = "dangquangthang";
const char* password = "123456";
#define dhtType DHT11

int lastMsg = 0;
char msg[100];
int ledPin = 2;
int fanPin = 15;
int m9biPin = 34;
int cds05Pin = 35;
int dhtPin = 32;

// Khai báo biến
WiFiClient wfClient;
PubSubClient psClient;
DHT dht(dhtPin, dhtType);
char txt[300];

const char* mqtt_pub = "esp/response";
const char* mqtt_sub = "esp/command";

void wifiConnect() {
  Serial.print("Connect to ");
  Serial.println(ssid);
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  };
  Serial.println();
  Serial.println("Connected");
  Serial.println("The IP address: ");
  Serial.println(WiFi.localIP());
}

void setup() {
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);
   pinMode(fanPin, OUTPUT);
  pinMode(m9biPin, INPUT);
  pinMode(cds05Pin, INPUT);
  digitalWrite(ledPin, LOW);
  digitalWrite(fanPin, LOW);
  dht.begin();
  wifiConnect();
  psClient.setClient(wfClient);
  psClient.setServer(host, port);
  psClient.setCallback(callback);
}
void callback(char* topic, byte* payload, unsigned int leng) {
  Serial.print("Received[");
  Serial.print(topic);
  Serial.println("]");
  for (int i = 0; i < leng; i++) {
    txt[i] = (char)payload[i];
  }
  txt[leng] = '\0';
  Serial.println(txt);
  StaticJsonDocument<48> jsonDocument;
  deserializeJson(jsonDocument, txt);
  String requestLed = jsonDocument["led"]["state"]["onOff"];
  if (requestLed == "true") {
    digitalWrite(ledPin, HIGH);
  } else if (requestLed == "false") {
    digitalWrite(ledPin, LOW);
  }
  String requestFan = jsonDocument["fan"]["state"]["onOff"];
  if (requestFan == "true") {
    digitalWrite(fanPin, HIGH);
  } else if (requestFan == "false") {
    digitalWrite(fanPin, LOW);
  }
}
void reconnect() {
  while (!psClient.connected()) {
    if (psClient.connect("MyESP32", username, password)) {
      Serial.println("Connected!");
      psClient.publish(mqtt_pub, "Reconnect");
      psClient.subscribe(mqtt_sub);
    } else {
      Serial.println("Failed to connect to server, rc = ");
      Serial.println(psClient.state());
      Serial.println("Wait for 5 seconds");
      delay(5000);
    }
  }
}
void loop() {
  if (!psClient.connected()) {
    reconnect();
  }
  psClient.loop();
  long now = millis();
  if (now - lastMsg > 10000) {
    lastMsg = now;
    float humdAna = dht.readHumidity();
    float humd = map(humdAna, 0, 1024, 0, 100);
    float temp = dht.readTemperature();
    float lightAna = analogRead(cds05Pin);
    float light = map(lightAna, 0, 1024, 0, 100);
    float moisAna = analogRead(m9biPin);
    float mois = map(moisAna, 0, 1024, 0, 100);
    if (isnan(humd)) {
      Serial.println("Failed to read data from DHT11");
      return;
    }
    if (isnan(light)) {
      Serial.println("Failed to read data from MS - CDS05");
      return;
    }
    if (isnan(mois)) {
      Serial.println("Failed to read data from MB05");
      return;
    }
    StaticJsonDocument<300> jsonDocument;
    JsonObject root = jsonDocument.to<JsonObject>();
    JsonObject temp_sensor = root.createNestedObject("temp");
    temp_sensor["temp"] = temp;
    JsonObject humid_sensor = root.createNestedObject("humid");
    humid_sensor["humid"] = humd;
    JsonObject light_sensor = root.createNestedObject("light");
    light_sensor["light"] = light;
    JsonObject mois_sensor = root.createNestedObject("mois");
    mois_sensor["mois"] = mois;
    serializeJson(jsonDocument, msg);
    psClient.publish(mqtt_pub, msg);
  }
}
