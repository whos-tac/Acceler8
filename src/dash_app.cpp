#include "dash_app.h"
#include <lvgl.h>
#include "display_driver.h"
#include "can_driver.h"
#include "ui_controller.h"
#include "underglow_controller.h"
#include "espnow_dash.h"
#include "odometer.h"

void DashApp::init() {
    DisplayDriver::init();
    CANDriver::init();
    UIController::init();
#ifdef ARDUINO
    UnderglowController::init();
#endif
    EspNowDash::init();
    Odometer::init();
}

void DashApp::update() {
    DisplayDriver::tick();
    CANDriver::poll();
    UIController::update();
#ifdef ARDUINO
    UnderglowController::update();
#endif
    EspNowDash::poll();
    
    // Explicit tick increment for LVGL is handled in main loop or sim loop
    Odometer::update();
}
