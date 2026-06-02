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

double total_distance = 0.0;
static double fractional_distance = 0.0;
static double last_saved_distance = 0.0;
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
    total_distance = preferences.getDouble("tot_dist", 0.0);
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
        float current_speed = calculate_speed_kmh(fabs((float)g_vehicle_state.erpm));
        
        // Cap the maximum allowable speed integration per tick (e.g., 200 km/h)
        if (current_speed > 200.0f) current_speed = 200.0f;

        fractional_distance += current_speed * dt_h;
        
        // Accumulate to total_distance to prevent precision loss, without a while loop
        if (fractional_distance >= 0.1) {
            double increments = floor(fractional_distance * 10.0) / 10.0;
            total_distance += increments;
            fractional_distance -= increments;
        }
        
        last_dist_update_ms = now;

#ifdef ARDUINO
        if ((total_distance - last_saved_distance) >= 1.0) {
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
        preferences.putDouble("tot_dist", total_distance);
        last_saved_distance = total_distance;
    }
#endif
}

void Odometer::reset() {
    total_distance = 0.0;
    fractional_distance = 0.0;
    last_saved_distance = -1.0;
    save_if_needed();
}
