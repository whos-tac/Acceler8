#include "dash_app.h"
#include <lvgl.h>
#include "display_driver.h"
#include "can_driver.h"
#include "ui_controller.h"
#include "underglow_controller.h"
#include "espnow_dash.h"

#ifdef ARDUINO
#include <Preferences.h>
static Preferences preferences;
#else
#include <SDL2/SDL.h>
#endif

float total_distance = 0.0f;
static float last_saved_distance = 0.0f;
static uint32_t last_dist_update_ms = 0;

static uint32_t get_millis() {
#ifdef ARDUINO
    return millis();
#else
    return SDL_GetTicks();
#endif
}

void DashApp::init() {
    DisplayDriver::init();
    CANDriver::init();
    UIController::init();
#ifdef ARDUINO
    UnderglowController::init();
    
    preferences.begin("dash", false);
    total_distance = preferences.getFloat("tot_dist", 0.0f);
    last_saved_distance = total_distance;
#endif
    EspNowDash::init();
    
    last_dist_update_ms = get_millis();
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

    // Update total_distance
    uint32_t now = get_millis();
    uint32_t dt_ms = now - last_dist_update_ms;
    if (dt_ms > 0 && dt_ms < 1000) { // sanity check
        float dt_h = dt_ms / 3600000.0f;
        total_distance += g_vehicle_state.speed_kmh * dt_h;
        last_dist_update_ms = now;

#ifdef ARDUINO
        if ((total_distance - last_saved_distance) >= 1.0f) {
            preferences.putFloat("tot_dist", total_distance);
            last_saved_distance = total_distance;
        }
#endif
    } else if (dt_ms >= 1000) {
        last_dist_update_ms = now; // reset if too much time passed
    }
}
