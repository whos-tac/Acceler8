#ifdef ARDUINO
#include <Arduino.h>
#endif

#include <lvgl.h>
#include "dash_app.h"

#ifdef ARDUINO

void setup() {
    Serial.begin(115200);
    delay(100);
    Serial.println("ESK8 Dashboard Executing...");

    DashApp::init();
}

void loop() {
    DashApp::update();
    
    // Explicit tick increment for LVGL
    lv_tick_inc(5);
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

    while(true) {
        DashApp::update();
        
        lv_tick_inc(5);
        SDL_Delay(5);
    }
    return 0;
}
#endif

#endif
