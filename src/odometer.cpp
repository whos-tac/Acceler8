#include "odometer.h"
#include "can_driver.h"
#include "mechanical_config.h"

#ifdef ARDUINO
#include <Arduino.h>
#include <Preferences.h>
static Preferences preferences;
#else
#include <SDL2/SDL.h>
#endif

float total_distance = 0.0f;
static float fractional_distance = 0.0f;
static float last_saved_distance = 0.0f;
static uint32_t last_dist_update_ms = 0;

static uint32_t get_millis() {
#ifdef ARDUINO
    return millis();
#else
    return SDL_GetTicks();
#endif
}

void Odometer::init() {
#ifdef ARDUINO
    preferences.begin("dash", false);
    total_distance = preferences.getFloat("tot_dist", 0.0f);
    last_saved_distance = total_distance;
#endif
    fractional_distance = 0.0f;
    last_dist_update_ms = get_millis();
}

void Odometer::update() {
    uint32_t now = get_millis();
    uint32_t dt_ms = now - last_dist_update_ms;
    
    if (dt_ms > 0 && dt_ms < 1000) { // sanity check
        float dt_h = dt_ms / 3600000.0f;
        fractional_distance += g_vehicle_state.speed_kmh * dt_h;
        
        // Accumulate to total_distance to prevent float precision loss
        while (fractional_distance >= 0.1f) {
            total_distance += 0.1f;
            fractional_distance -= 0.1f;
        }
        
        last_dist_update_ms = now;

#ifdef ARDUINO
        if ((total_distance - last_saved_distance) >= 1.0f) {
            save_if_needed();
        }
#endif
    } else if (dt_ms >= 1000) {
        last_dist_update_ms = now; // reset if too much time passed
    }
}

void Odometer::save_if_needed() {
#ifdef ARDUINO
    if (total_distance != last_saved_distance) {
        preferences.putFloat("tot_dist", total_distance);
        last_saved_distance = total_distance;
    }
#endif
}
