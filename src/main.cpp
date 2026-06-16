#ifdef ARDUINO
#include <Arduino.h>
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
