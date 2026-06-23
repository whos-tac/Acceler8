#include "simulation/esc_model.h"
#include <iostream>
#include <cassert>
#include <cmath>
#include <limits>

// ponytail: basic assert helper that prints test details
#define ASSERT_NEAR(val1, val2, eps) \
    do { \
        if (std::abs((val1) - (val2)) > (eps)) { \
            std::cerr << "Assertion failed: " << #val1 << " (" << (val1) << ") near " << #val2 << " (" << (val2) << ") at " << __FILE__ << ":" << __LINE__ << std::endl; \
            return false; \
        } \
    } while(0)

#define ASSERT_TRUE(cond) \
    do { \
        if (!(cond)) { \
            std::cerr << "Assertion failed: " << #cond << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
            return false; \
        } \
    } while(0)

#define ASSERT_FALSE(cond) \
    do { \
        if ((cond)) { \
            std::cerr << "Assertion failed: !" << #cond << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
            return false; \
        } \
    } while(0)

bool test_throttle_mapping() {
    std::cout << "Running test_throttle_mapping..." << std::endl;

    float center = 2048.0f;
    float min_val = 1000.0f;
    float max_val = 3000.0f;

    // 1. Center is deadband, should map to 0
    float t_center = SimCore::map_pot_to_throttle(center, center, min_val, max_val);
    ASSERT_NEAR(t_center, 0.0f, 0.01f);

    // 2. Deadband bounds (+- 100 around center)
    float t_db_high = SimCore::map_pot_to_throttle(center + 100.0f, center, min_val, max_val);
    ASSERT_NEAR(t_db_high, 0.0f, 0.01f);

    float t_db_low = SimCore::map_pot_to_throttle(center - 100.0f, center, min_val, max_val);
    ASSERT_NEAR(t_db_low, 0.0f, 0.01f);

    // 3. Just outside deadband
    float t_outside_high = SimCore::map_pot_to_throttle(center + 101.0f, center, min_val, max_val);
    ASSERT_TRUE(t_outside_high > 0.0f);

    float t_outside_low = SimCore::map_pot_to_throttle(center - 101.0f, center, min_val, max_val);
    ASSERT_TRUE(t_outside_low < 0.0f);

    // 4. Maximum values
    float t_max = SimCore::map_pot_to_throttle(max_val, center, min_val, max_val);
    ASSERT_NEAR(t_max, 100.0f, 0.01f);

    float t_min = SimCore::map_pot_to_throttle(min_val, center, min_val, max_val);
    ASSERT_NEAR(t_min, -100.0f, 0.01f);

    // 5. Midpoint mapping
    float db_high = center + 100.0f;
    float mid_high = db_high + (max_val - db_high) / 2.0f;
    float t_mid_high = SimCore::map_pot_to_throttle(mid_high, center, min_val, max_val);
    ASSERT_NEAR(t_mid_high, 50.0f, 0.01f);

    std::cout << "test_throttle_mapping PASSED" << std::endl;
    return true;
}

bool test_ramping_failsafe() {
    std::cout << "Running test_ramping_failsafe..." << std::endl;

    float ramped = 0.0f;
    bool safe_start = false;

    // 1. Safe start logic: input throttle must be within deadband to enable safe_start
    SimCore::calculate_ramped_throttle(50.0f, ramped, 0.1f, false, false, safe_start);
    ASSERT_FALSE(safe_start);
    ASSERT_NEAR(ramped, 0.0f, 0.01f); // Should not ramp up before safe start

    SimCore::calculate_ramped_throttle(0.0f, ramped, 0.1f, false, false, safe_start);
    ASSERT_TRUE(safe_start);

    // 2. Standard acceleration ramp (75% / sec)
    // Starting at 0%, target is 100%. After 1 second, it should be 75.0%
    SimCore::calculate_ramped_throttle(100.0f, ramped, 1.0f, false, false, safe_start);
    ASSERT_NEAR(ramped, 75.0f, 0.01f);

    // 3. Fast decay on brake/deceleration (500% / sec)
    // Target is 0%, starting at 75%. After 0.1 seconds, it should decay by 50% to 25%
    SimCore::calculate_ramped_throttle(0.0f, ramped, 0.1f, false, false, safe_start);
    ASSERT_NEAR(ramped, 25.0f, 0.01f);

    // 4. Coast decay during signal loss (200% / sec)
    // Target is 0%, starting at 25%, signal_lost = true. After 0.1 seconds, decay by 20% to 5%
    ramped = 25.0f;
    SimCore::calculate_ramped_throttle(100.0f, ramped, 0.1f, true, false, safe_start);
    ASSERT_NEAR(ramped, 5.0f, 0.01f);
    ASSERT_FALSE(safe_start); // safe_start reset to false on signal loss

    std::cout << "test_ramping_failsafe PASSED" << std::endl;
    return true;
}

bool test_battery_model() {
    std::cout << "Running test_battery_model..." << std::endl;

    SimCore::SimState state;
    state.ramped_throttle = 100.0f; // 100% throttle
    state.capacity_ah = 10.0f; // Full capacity
    state.lvc_active = false;
    state.erpm = 0.0f;

    // 1. Initial update: check current calculation and voltage sag
    // 100% throttle -> duty cycle = 1.0 -> motor_current = 50A -> battery_current = 50A
    // Voltage sag = 50 * 0.1 = 5.0V
    // OCV at 10.0Ah = 42.0V -> loaded_voltage = 37.0V
    SimCore::update_esc_physics(state, 0.0f, false);
    ASSERT_NEAR(state.motor_current, 50.0f, 0.01f);
    ASSERT_NEAR(state.battery_current, 50.0f, 0.01f);
    ASSERT_NEAR(state.voltage_sag, 5.0f, 0.01f);
    ASSERT_NEAR(state.battery_voltage, 37.0f, 0.01f);
    ASSERT_NEAR(state.power_w, 37.0f * 50.0f, 0.01f);

    // 2. Capacity drain
    // Update over 72 seconds (72s = 0.02 hours)
    // Battery current = 50A -> drain = 50A * 0.02h = 1.0 Ah
    // Capacity should become 9.0 Ah
    SimCore::update_esc_physics(state, 72.0f, false);
    ASSERT_NEAR(state.capacity_ah, 9.0f, 0.01f);

    // 3. Rapid drain multiplier
    // With rapid drain = true, drain current = base_drain (50A) + 15A = 65A
    // multiplied by 30x = 1950 A equivalent drain
    // Update over 1.8 seconds (1.8s = 0.0005h)
    // drain = 1950A * 0.0005h = 0.975 Ah
    // Capacity should decrease from 9.0Ah to ~8.025 Ah
    SimCore::update_esc_physics(state, 1.8f, true);
    ASSERT_NEAR(state.capacity_ah, 8.025f, 0.01f);

    // 4. Low-Voltage Cutoff (LVC)
    // Force capacity down close to 0 to trigger LVC (LVC at 32V)
    state.capacity_ah = 0.5f; // OCV = 30 + 12 * 0.05 = 30.6V
    // Update physics: even without load, OCV is 30.6V < 32.0V, so it should trigger LVC immediately
    SimCore::update_esc_physics(state, 0.1f, false);
    ASSERT_TRUE(state.lvc_active);
    ASSERT_NEAR(state.motor_current, 0.0f, 0.01f);
    ASSERT_NEAR(state.battery_current, 0.0f, 0.01f);
    ASSERT_NEAR(state.battery_voltage, 30.6f, 0.01f); // Recovered to OCV since current is 0

    // 5. Recharge / Recovery
    state.capacity_ah = 10.0f; // recharge
    SimCore::update_esc_physics(state, 0.1f, false);
    ASSERT_FALSE(state.lvc_active); // should recover

    std::cout << "test_battery_model PASSED" << std::endl;
    return true;
}

bool test_lvc_chatter_fix() {
    std::cout << "Running test_lvc_chatter_fix..." << std::endl;
    SimCore::SimState state;
    state.ramped_throttle = 100.0f;
    state.capacity_ah = 0.5f; // low capacity, triggers LVC

    // Run one update to trigger LVC
    SimCore::update_esc_physics(state, 0.1f, false);
    ASSERT_TRUE(state.lvc_active);
    ASSERT_NEAR(state.motor_current, 0.0f, 0.01f);
    ASSERT_NEAR(state.battery_voltage, 30.6f, 0.01f); // Recovered voltage = 30.6V

    // Now, even if capacity is slightly increased to make voltage recover above 33.0V (e.g. capacity = 3.0Ah, voltage = 30 + 12 * 0.3 = 33.6V)
    // LVC should still be active because capacity is not >= 9.9 Ah
    state.capacity_ah = 3.0f;
    SimCore::update_esc_physics(state, 0.1f, false);
    ASSERT_TRUE(state.lvc_active);

    // Now, recharge capacity to 9.9 Ah
    state.capacity_ah = 9.9f;
    SimCore::update_esc_physics(state, 0.1f, false);
    ASSERT_FALSE(state.lvc_active); // Should recover now!

    std::cout << "test_lvc_chatter_fix PASSED" << std::endl;
    return true;
}

bool test_integration_stability() {
    std::cout << "Running test_integration_stability..." << std::endl;
    SimCore::SimState state;
    std::cout << "Before update:" << std::endl;
    std::cout << "  ramped_throttle: " << state.ramped_throttle << std::endl;
    std::cout << "  erpm: " << state.erpm << std::endl;
    std::cout << "  capacity_ah: " << state.capacity_ah << std::endl;
    std::cout << "  lvc_active: " << state.lvc_active << std::endl;
    std::cout << "  battery_voltage: " << state.battery_voltage << std::endl;
    
    state.ramped_throttle = 100.0f;
    state.erpm = 0.0f;

    // Call update_esc_physics with a very large dt = 10.0 seconds.
    // Analytical integration guarantees it stays stable.
    SimCore::update_esc_physics(state, 10.0f, false);
    
    std::cout << "After update:" << std::endl;
    std::cout << "  ramped_throttle: " << state.ramped_throttle << std::endl;
    std::cout << "  erpm: " << state.erpm << std::endl;
    std::cout << "  capacity_ah: " << state.capacity_ah << std::endl;
    std::cout << "  lvc_active: " << state.lvc_active << std::endl;
    std::cout << "  battery_voltage: " << state.battery_voltage << std::endl;

    ASSERT_TRUE(state.erpm > 0.0f);
    ASSERT_TRUE(state.erpm <= 80000.0f + 0.1f);
    ASSERT_NEAR(state.erpm, 80000.0f, 1.0f); // Should have converged to target erpm

    // 2. Test large timestep dt = 72.0s
    state.erpm = 0.0f;
    state.capacity_ah = 10000.0f; // Large capacity to prevent LVC
    SimCore::update_esc_physics(state, 72.0f, false);
    ASSERT_TRUE(state.erpm > 0.0f);
    ASSERT_TRUE(state.erpm <= 80000.0f + 0.1f);
    ASSERT_NEAR(state.erpm, 80000.0f, 0.001f);

    // 3. Test extremely large timestep dt = 100000.0s
    state.erpm = 0.0f;
    state.capacity_ah = 10000000.0f; // Large capacity to prevent LVC
    SimCore::update_esc_physics(state, 100000.0f, false);
    ASSERT_TRUE(state.erpm > 0.0f);
    ASSERT_TRUE(state.erpm <= 80000.0f + 0.1f);
    ASSERT_NEAR(state.erpm, 80000.0f, 0.001f);

    // 4. Test dt = 0.0s
    state.erpm = 50000.0f;
    state.capacity_ah = 10.0f;
    SimCore::update_esc_physics(state, 0.0f, false);
    ASSERT_NEAR(state.erpm, 50000.0f, 0.001f);

    std::cout << "test_integration_stability PASSED" << std::endl;
    return true;
}

bool test_brake_throttle_scaling() {
    std::cout << "Running test_brake_throttle_scaling..." << std::endl;
    float ramped = 0.0f;
    bool safe_start = true;

    // Input negative throttle: -50%
    // Target throttle = -((50 - 10) / 90) * 100% = -44.44%
    // Scaled by (50.0/50.0) = -44.44%
    SimCore::calculate_ramped_throttle(-50.0f, ramped, 1.0f, false, false, safe_start);
    ASSERT_NEAR(ramped, -44.44f, 0.1f);

    std::cout << "test_brake_throttle_scaling PASSED" << std::endl;
    return true;
}

bool test_nan_sanitization() {
    std::cout << "Running test_nan_sanitization..." << std::endl;
    float ramped = 0.0f;
    bool safe_start = true;

    // input NAN throttle, should not crash, should be treated as 0.0f
    float nan_val = std::numeric_limits<float>::quiet_NaN();
    SimCore::calculate_ramped_throttle(nan_val, ramped, 0.1f, false, false, safe_start);
    ASSERT_NEAR(ramped, 0.0f, 0.01f);

    // input NAN to map_pot_to_throttle, should map to 0 (since it uses center)
    float t_nan = SimCore::map_pot_to_throttle(nan_val, 2048.0f, 1000.0f, 3000.0f);
    ASSERT_NEAR(t_nan, 0.0f, 0.01f);

    std::cout << "test_nan_sanitization PASSED" << std::endl;
    return true;
}

int main() {
    bool success = true;
    success &= test_throttle_mapping();
    success &= test_ramping_failsafe();
    success &= test_battery_model();
    success &= test_lvc_chatter_fix();
    success &= test_integration_stability();
    success &= test_brake_throttle_scaling();
    success &= test_nan_sanitization();

    if (success) {
        std::cout << "ALL TESTS PASSED SUCCESSFULLY!" << std::endl;
        return 0;
    } else {
        std::cerr << "SOME TESTS FAILED!" << std::endl;
        return 1;
    }
}
