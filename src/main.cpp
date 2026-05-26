#include <Arduino.h>
#include <lvgl.h>
#include "display_driver.h"
#include "can_driver.h"
#include "ui_controller.h"

void setup() {
    Serial.begin(115200);
    delay(100);
    Serial.println("ESK8 Dashboard Executing...");

    DisplayDriver::init();
    CANDriver::init();
    UIController::init();
}

void loop() {
    DisplayDriver::tick();
    CANDriver::poll();
    UIController::update();
    
    // Explicit tick increment for LVGL
    lv_tick_inc(5);
    delay(5);
}
