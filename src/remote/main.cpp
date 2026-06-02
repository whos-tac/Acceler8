#include <Arduino.h>
#include "remote_app.h"
#include <lvgl.h>

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
    static uint32_t last_tick = millis();
    uint32_t current_time = millis();
    uint32_t delta = current_time - last_tick;
    last_tick = current_time;

    RemoteApp::update();
    lv_tick_inc(delta);
    lv_timer_handler();
    delay(10);
}
#endif
