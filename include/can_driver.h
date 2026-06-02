#pragma once
#ifdef ARDUINO
#include <Arduino.h>
#include <stdint.h>
#include <stdbool.h>
#endif

#ifdef ARDUINO
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
extern SemaphoreHandle_t dash_mutex;
#define DASH_LOCK() do { if(dash_mutex) xSemaphoreTake(dash_mutex, portMAX_DELAY); } while(0)
#define DASH_UNLOCK() do { if(dash_mutex) xSemaphoreGive(dash_mutex); } while(0)
#else
#define DASH_LOCK()
#define DASH_UNLOCK()
#endif

// Global Telemetry State (Based on VESC standard)
struct VehicleState {
    // --- Raw CAN data (from REALTIME_DATA frames) ---
    int32_t erpm;
    float current_a;            // Motor current (A) — REALTIME_DATA_0
    float duty_cycle;           // Duty cycle 0.0–1.0 — REALTIME_DATA_1
    float mosfet_temp_c;        // ESC MOSFET temp — REALTIME_DATA_2
    float motor_temp_c;         // Motor temp — REALTIME_DATA_2
    float battery_current_a;    // Battery current (A), negative = regen — REALTIME_DATA_0
    float battery_voltage_v;    // Battery voltage — REALTIME_DATA_2
    int32_t tachometer;         // Cumulative tach counts
    float speed_kmh;            // Current speed in km/h

    // --- Derived / computed metrics ---
    float power_w;              // Instantaneous watts (voltage × battery_current)
    float wh_consumed;          // Cumulative watt-hours this session
    float top_speed_kmh;        // Session peak speed
    float avg_speed_kmh;        // Running average speed
    float max_current_a;        // Session peak motor current
    float range_km;             // Estimated remaining range
    float wh_per_km;            // Energy efficiency (Wh/km)
    bool  regen_active;         // True when battery_current < 0

    // --- System status ---
    bool  can_alive;            // CAN frames being received
    bool  has_received_can;     // True if at least one CAN frame was ever received
    uint32_t last_can_rx_ms;    // Timestamp of last CAN frame
    bool remote_disconnected;
    uint8_t remote_button_state; // From Remote ControlPacket

    // --- Underglow Control ---
    uint8_t led_r;
    uint8_t led_g;
    uint8_t led_b;
    uint8_t led_brightness;
    uint8_t led_mode;           // 0: Off, 1: Solid, 2: Breathing, 3: Speed-Reactive

    // --- Internal accumulators (for avg computation) ---
    uint32_t speed_sample_count;
    double speed_sum;
    uint32_t last_wh_update_ms; // For Wh integration

    bool mock_mode_active = false;
};

// Extern declaration so the UI can safely read it
extern VehicleState g_vehicle_state;

// Total battery capacity — adjust for your pack
#define BATTERY_TOTAL_WH 360.0f   // e.g., 10S4P 36V 10Ah = 360Wh

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
