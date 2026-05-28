#include <Arduino.h>
#include "remote_app.h"

#ifdef ARDUINO
#include <WiFi.h>
#include <esp_now.h>
#include <TFT_eSPI.h>

TFT_eSPI tft = TFT_eSPI();

void setup() {
    Serial.begin(115200);
    delay(100);
    RemoteApp::init();
}

void loop() {
    RemoteApp::update();
    delay(10);
}
#endif
