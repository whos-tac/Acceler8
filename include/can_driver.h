#pragma once
#include <Arduino.h>

// Global Telemetry State (Based on VESC standard)
struct VehicleState {
    int32_t erpm;
    float current_a;
    float duty_cycle;
    float mosfet_temp_c;
    float motor_temp_c;
    float battery_current_a;
    float battery_voltage_v;
    int32_t tachometer;
};

// Extern declaration so the UI can safely read it
extern VehicleState g_vehicle_state;

namespace CANDriver {
    /**
     * @brief Initialize ESP32 TWAI (CAN) hardware on TX: 6, RX: 0.
     */
    void init();

    /**
     * @brief Polls for new CAN messages and updates g_vehicle_state.
     * Thread-safe updates to global state can be performed here.
     */
    void poll();
}
