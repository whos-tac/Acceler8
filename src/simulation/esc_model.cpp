#include "esc_model.h"
#include <cmath>
#include <algorithm>

namespace SimCore {

// ponytail: basic deadband scaling mapping
float map_pot_to_throttle(float pot_val, float pot_center, float pot_min, float pot_max) {
    if (std::isnan(pot_val)) {
        pot_val = pot_center;
    }
    float pot_f = pot_val;
    if (pot_f > pot_max) pot_f = pot_max;
    if (pot_f < pot_min) pot_f = pot_min;

    float deadband_high = pot_center + 100.0f;
    float deadband_low = pot_center - 100.0f;
    float throttle = 0.0f;

    if (pot_f > deadband_high) {
        float denom = pot_max - deadband_high;
        if (denom >= 10.0f) {
            throttle = ((pot_f - deadband_high) / denom) * 100.0f;
        }
    } else if (pot_f < deadband_low) {
        float denom = deadband_low - pot_min;
        if (denom >= 10.0f) {
            throttle = ((pot_f - deadband_low) / denom) * 100.0f; // negative
        }
    }
    
    if (throttle > 100.0f) throttle = 100.0f;
    if (throttle < -100.0f) throttle = -100.0f;
    return throttle;
}

// ponytail: ramp and failsafe calculation matching receiver app requirements
void calculate_ramped_throttle(float input_throttle, float &ramped_throttle, float dt, bool signal_lost, bool settings_active, bool &safe_start) {
    if (std::isnan(input_throttle)) {
        input_throttle = 0.0f;
    }
    if (signal_lost) {
        safe_start = false;
    }
    if (!signal_lost && std::abs(input_throttle) <= 10.0f /* THROTTLE_DEADZONE */) {
        safe_start = true;
    }

    float target = 0.0f;
    if (signal_lost || settings_active || !safe_start) {
        target = 0.0f;
    } else {
        if (std::abs(input_throttle) <= 10.0f) {
            target = 0.0f;
        } else {
            float sign = (input_throttle > 0.0f) ? 1.0f : -1.0f;
            target = sign * (std::abs(input_throttle) - 10.0f) * (100.0f / (100.0f - 10.0f));
        }
        if (target < 0.0f) {
            // ponytail: scale target for brake throttle to match receiver app's scaling using MAX_BRAKE_CURRENT_A / MAX_DRIVE_CURRENT_A
            target *= (50.0f / 50.0f);
        }
    }

    float time_left = dt;
    int iterations = 0;
    while (time_left > 0.0f && ramped_throttle != target && iterations < 10) {
        iterations++;
        float current_rate;
        if (signal_lost || settings_active || !safe_start) {
            current_rate = 200.0f; // FAILSAFE_COAST_RATE
        } else {
            bool accelerating = false;
            if (target > 0.0f && target > ramped_throttle && ramped_throttle >= 0.0f) {
                accelerating = true;
            } else if (target < 0.0f && target < ramped_throttle && ramped_throttle <= 0.0f) {
                accelerating = true;
            }
            current_rate = accelerating ? 75.0f /* RAMP_RATE_PER_SEC */ : 500.0f /* RAMP_DOWN_RATE_PER_SEC */;
        }

        float max_delta = current_rate * time_left;

        if (ramped_throttle < target) {
            if (ramped_throttle < 0.0f && target >= 0.0f) {
                float dist = 0.0f - ramped_throttle;
                if (max_delta > dist) {
                    ramped_throttle = 0.0f;
                    time_left -= dist / current_rate;
                    continue;
                }
            }
            ramped_throttle += max_delta;
            if (ramped_throttle > target) ramped_throttle = target;
            break;
        } else {
            if (ramped_throttle > 0.0f && target <= 0.0f) {
                float dist = ramped_throttle - 0.0f;
                if (max_delta > dist) {
                    ramped_throttle = 0.0f;
                    time_left -= dist / current_rate;
                    continue;
                }
            }
            ramped_throttle -= max_delta;
            if (ramped_throttle < target) ramped_throttle = target;
            break;
        }
    }
}

// ponytail: battery model & ESC physics integration
void update_esc_physics(SimState &state, float dt, bool rapid_drain) {
    // 1. Calculate active capacity drain (max 10 Ah)
    float base_drain = state.battery_current;
    float drain_multiplier = 1.0f;
    if (rapid_drain) {
        base_drain += 15.0f; // Drains 15A at idle
        drain_multiplier = 30.0f; // Scale up for visual feedback/test speed
    }
    float total_drain = base_drain * drain_multiplier;
    state.capacity_ah -= (total_drain * dt) / 3600.0f;
    if (state.capacity_ah < 0.0f) state.capacity_ah = 0.0f;

    // 2. Open circuit voltage based on capacity (linear between 30V at 0Ah and 42V at 10Ah)
    float max_capacity = 10.0f;
    float ocv = 30.0f + 12.0f * (state.capacity_ah / max_capacity);
    if (ocv < 30.0f) ocv = 30.0f;
    if (ocv > 42.0f) ocv = 42.0f;

    // 3. Compute motor current and battery current from ramped throttle
    float duty_cycle = std::abs(state.ramped_throttle) / 100.0f;

    if (state.lvc_active) {
        state.motor_current = 0.0f;
        state.battery_current = 0.0f;
    } else {
        state.motor_current = duty_cycle * 50.0f; // Max 50A
        state.battery_current = state.motor_current * duty_cycle;
    }

    // 4. Calculate voltage sag based on battery current and internal resistance (R_int = 0.1 Ohm)
    state.voltage_sag = state.battery_current * 0.1f;
    state.battery_voltage = ocv - state.voltage_sag;

    // 5. Low Voltage Cutoff (LVC) check at 32.0V
    if (state.battery_voltage < 32.0f) {
        state.lvc_active = true;
        state.motor_current = 0.0f;
        state.battery_current = 0.0f;
        state.voltage_sag = 0.0f;
        state.battery_voltage = ocv; // recovers to OCV when load is removed
    }

    // ponytail: LVC should only be cleared when a recharge/reset capacity event occurs (capacity >= 9.9 Ah)
    if (state.lvc_active && state.capacity_ah >= 9.9f) {
        state.lvc_active = false;
    }

    // 6. Calculate electrical power
    state.power_w = state.battery_voltage * state.battery_current;

    // 7. Update ERPM based on throttle, with lag (analytical solution to guarantee stability under large dt inputs)
    float target_erpm = 0.0f;
    if (!state.lvc_active) {
        target_erpm = (state.ramped_throttle / 100.0f) * 80000.0f;
    }
    state.erpm = target_erpm - (target_erpm - state.erpm) * std::exp(-2.0f * dt);
}

}
