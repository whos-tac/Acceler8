#include <Arduino.h>
#if defined(ESP8266)
#include <ESP8266WiFi.h>
#else
#include <WiFi.h>
#endif

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    // Set Wi-Fi to station mode
    WiFi.mode(WIFI_STA);
    
    // Print MAC Address
    Serial.println();
    Serial.println("=====================================");
    Serial.print("ESP Board MAC Address: ");
    Serial.println(WiFi.macAddress());
    Serial.println("=====================================");
    Serial.println("Copy this MAC address and save it.");
}

void loop() {
    // Nothing to do
    delay(1000);
}
