#pragma once
#include <cmath>

// ponytail: Pure C++ ESC and battery model
namespace SimCore {

struct SimState {
    float ramped_throttle = 0.0f;
    float erpm = 0.0f;
    float motor_current = 0.0f;
    float battery_current = 0.0f;
    float battery_voltage = 42.0f;
    float capacity_ah = 10.0f; // Max 10.0 Ah
    float voltage_sag = 0.0f;
    float power_w = 0.0f;
    bool lvc_active = false;
    bool safe_start = false;
};

// Map analog potentiometer reading to throttle percentage (-100.0f to 100.0f)
float map_pot_to_throttle(float pot_val, float pot_center, float pot_min, float pot_max);

// Compute target throttle and apply acceleration ramping, decay rates, and failsafe coasting
void calculate_ramped_throttle(float input_throttle, float &ramped_throttle, float dt, bool signal_lost, bool settings_active, bool &safe_start);

// Update ERPM, motor current, battery current, battery capacity, voltage sag, power, and low-voltage cutoff
void update_esc_physics(SimState &state, float dt, bool rapid_drain);

}
