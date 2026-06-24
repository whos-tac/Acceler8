#ifdef ARDUINO
#include <Arduino.h>
#include <esp_sleep.h>
#include <Wire.h>
#endif

#include <lvgl.h>
#include "dash_app.h"

#include "can_driver.h"

#ifdef ARDUINO
SemaphoreHandle_t dash_mutex = NULL;

void setup() {
    Serial.begin(115200);
    delay(100);

    dash_mutex = xSemaphoreCreateMutex();

    DashApp::init();
}

void loop() {
    static uint32_t last_tick = millis();
    
    DashApp::update();
    
    uint32_t now = millis();
    uint32_t dt = now - last_tick;
    last_tick = now;
    
    lv_tick_inc(dt);
    delay(5);

    // ponytail: Deep sleep if no CAN traffic for 5 minutes
    if (g_vehicle_state.has_received_can && (now - g_vehicle_state.last_can_rx_ms > 300000)) {
        // Turn off 5V subsystem & backlight via TCA9554 IO Expander
        Wire.beginTransmission(0x24);
        Wire.write(0x01);
        Wire.write(0x0B); // Set Pin 0 HIGH (VBAT_5V Disable)
        Wire.endTransmission();

        // Wake up when CAN_RX_PIN (GPIO 0) goes LOW
        esp_sleep_enable_ext1_wakeup(1ULL << 0, ESP_EXT1_WAKEUP_ANY_LOW);
        esp_deep_sleep_start();
    }
}

#else

#ifndef NATIVE_FULL_STACK
// Native Windows execution entry point for standalone dash
#include <unistd.h>
#include <SDL2/SDL.h>

int main(int argc, char **argv) {
    (void)argc; // Unused
    (void)argv; // Unused

    DashApp::init();

    uint32_t last_tick = SDL_GetTicks();
    while(true) {
        DashApp::update();
        
        uint32_t now = SDL_GetTicks();
        uint32_t dt = now - last_tick;
        last_tick = now;
        
        lv_tick_inc(dt);
        SDL_Delay(5);
    }
    return 0;
}
#endif

#endif
