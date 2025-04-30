#include <ESP8266WiFi.h>        // Wifi library
#include <PubSubClient.h>       // MQTT library

#include <DNSServer.h>            //Local DNS Server used for redirecting all requests to the configuration portal
#include <ESP8266WebServer.h>     //Local WebServer used to serve the configuration portal
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager WiFi Configuration Magic 

#include "config.h"

WiFiClient wifiClient;               // WiFi
PubSubClient client(wifiClient);     // MQTT

// PINS
const int BUTTON_PIN = 3;
const int LED_PIN = 2;

bool coffeeStatus = false;
bool triggered = false;
bool buttonPressed = false;

const int DEBOUNCE = 250;
unsigned long buttonTimer = 0;

bool isConnected = false;

void setup() {

  pinMode(LED_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT);

  WiFiManager wifiManager;
  WiFiManagerParameter custom_text("<p>(c) 2018 by <a href=\"mailto:hoi@joszuijderwijk.nl\">Jos Zuijderwijk</a></p>");
  wifiManager.addParameter(&custom_text);

  if (wifiManager.autoConnect("KoffieKnop", "")){
    isConnected = true;
  }
  
  client.setServer(MQTT_SERVER, MQTT_PORT);
  client.setCallback(callback);

  // turn off LED
  digitalWrite(LED_PIN, HIGH);

}

// Handel inkomenden berichten af
void callback(char* topic, byte* payload, unsigned int len) {
    
    String msg = ""; // payload
    for (int i = 0; i < len; i++) {
      msg += ((char)payload[i]);
    }

  if ( strcmp(topic, "coffee/status") == 0 ){
      // start
      triggered = true;
      coffeeStatus = (msg == "1");
      Serial.println("new status: ");
      Serial.print(coffeeStatus ? "1" : "0");
  }
   
}

// Probeer MQTT connectie te herstellen
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");

    
    if (client.connect(MQTT_CLIENT, MQTT_USER, MQTT_PASS, "coffee/button/connection", 0, 1, "0")) {
      Serial.println("connected");

      // Stuur Hello World!
      client.publish("coffee/button/connection", "1", 1);
      client.subscribe("coffee/status");
      
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");

    }
  }
}

// the loop function runs over and over again forever
void loop() {
    if (!client.connected() && isConnected){
     reconnect();
    }

    if (triggered){
      triggered = false;
      
      // change LED
      if (coffeeStatus)
        digitalWrite(LED_PIN, LOW);
      else
        digitalWrite(LED_PIN, HIGH);
    }

    bool currentlyPressed = digitalRead(BUTTON_PIN);
    
    if (currentlyPressed && !buttonPressed && millis() - buttonTimer > DEBOUNCE){
       buttonTimer = millis();
       buttonPressed = true;
       coffeeStatus = !coffeeStatus;
       client.publish("coffee/start", coffeeStatus ? "1" : "0", 0);
    }
    
    if (!currentlyPressed)
      buttonPressed = false;
         
    client.loop();    
}
