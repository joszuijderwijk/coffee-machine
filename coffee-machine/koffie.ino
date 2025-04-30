#include <ESP8266WiFi.h>        // Wifi library
#include <PubSubClient.h>       // MQTT library
#include <DallasTemperature.h> // temp
#include <OneWire.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager WiFi Configuration Magic 

#include "config.h"

WiFiClient wifiClient;               // WiFi
PubSubClient client(wifiClient);     // MQTT

// PINS
const int TEMP_PIN = D2;
const int COFFEE_PIN = D1;
const int LED_PIN = A0;

// Sensors
OneWire oneWire(TEMP_PIN);
DallasTemperature sensors(&oneWire);

// Timers
const int CheckStatusInterval = 1000;
const int SendTempInterval = 5000;

unsigned long statTimer = 0;
unsigned long tempTimer = 0;

float Temp = 0;

bool triggerCoffee = false;
bool firstTrigger = true;
bool coffeeStatus = false;

bool isConnected = false;

void setup() {

  WiFiManager wifiManager;
  WiFiManagerParameter custom_text("<p>(c) 2018 by <a href=\"mailto:dev@joszuijderwijk.nl\">Jos Zuijderwijk</a></p>");
  wifiManager.addParameter(&custom_text);

  if (wifiManager.autoConnect("KoffiePot", "")){
    isConnected = true;
  }
  
  client.setServer(MQTT_SERVER, MQTT_PORT);
  client.setCallback(callback);
  
}

// Handel inkomenden berichten af
void callback(char* topic, byte* payload, unsigned int len) {
    
    String msg = ""; // payload
    for (int i = 0; i < len; i++) {
      msg += ((char)payload[i]);
    }

  if ( strcmp(topic, "coffee/start") == 0 ){
      // start
      coffeeStatus = (msg == "1");
      triggerCoffee = true;
  }
   
}

// Probeer MQTT connectie te herstellen
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");

    if (client.connect(MQTT_CLIENT, MQTT_USER, MQTT_PASS, "coffee/connection", 0, 1, "0")) {
      Serial.println("connected");

      // Stuur Hello World!
      client.publish("coffee/connection", "1", 1);
      client.subscribe("coffee/start");

    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
    }
  }
}


void loop() {
    if (!client.connected()){
     reconnect();
    }

  if (triggerCoffee){
    triggerCoffee = false;

    client.publish("coffee/status", coffeeStatus ? "1" : "0", 1);

    //double check
    if (analogRead(LED_PIN) > 500 != coffeeStatus){
      digitalWrite(COFFEE_PIN, HIGH);
      delay(500);
      digitalWrite(COFFEE_PIN, LOW);
    }
  }

  if (millis() - statTimer > CheckStatusInterval){
    statTimer = millis();

    if ((analogRead(LED_PIN) > 500) != coffeeStatus){
      coffeeStatus = !coffeeStatus;
      client.publish("coffee/status", coffeeStatus ? "1" : "0", 1);

    }
  }

  // send temp
  if (millis() - tempTimer > SendTempInterval){
    tempTimer = millis();
    sensors.requestTemperatures();
    float currentTemp = sensors.getTempCByIndex(0);
    if (currentTemp != Temp){
      Temp = currentTemp;
      client.publish("coffee/temp", String(currentTemp).c_str(), 1);
    }
    
  }
  
  client.loop();

}
