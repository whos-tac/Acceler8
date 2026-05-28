#include "can_driver.h"
#include "mechanical_config.h"
#include <math.h>
#include <stdint.h>
#include <string.h>

#ifdef ARDUINO
#include <Arduino.h>
#include <driver/twai.h>
#include <esp_task_wdt.h>
#else
#include <SDL2/SDL.h>
static uint32_t millis() {
    return SDL_GetTicks();
}
#endif

VehicleState g_vehicle_state;

// Fallback mechanical configuration variables (can be modified by Settings UI)
int motor_pole_pairs = 7;
float gear_ratio = 4.0f;
float wheel_diameter_mm = 200.0f;

namespace CANDriver {

    void init() {
        memset(&g_vehicle_state, 0, sizeof(VehicleState));

#ifdef ARDUINO
        // Initialize ESP32 TWAI (CAN) hardware
        twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT((gpio_num_t)6, (gpio_num_t)0, TWAI_MODE_NORMAL);
        twai_timing_config_t t_config = TWAI_TIMING_CONFIG_1MBITS();
        twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

        if (twai_driver_install(&g_config, &t_config, &f_config) == ESP_OK) {
            twai_start();
        }
        esp_task_wdt_add(NULL);
#endif

        // Simulator/Default Initialization: Base values
        g_vehicle_state.battery_voltage_v = 42.0f;
        g_vehicle_state.mosfet_temp_c = 25.0f;
        g_vehicle_state.motor_temp_c = 25.0f;
        g_vehicle_state.erpm = 0;
        g_vehicle_state.battery_current_a = 0.0f;
        g_vehicle_state.current_a = 0.0f;
        g_vehicle_state.tachometer = 0;
        g_vehicle_state.duty_cycle = 0.0f;

        // Derived init
        g_vehicle_state.power_w = 0.0f;
        g_vehicle_state.wh_consumed = 0.0f;
        g_vehicle_state.top_speed_kmh = 0.0f;
        g_vehicle_state.avg_speed_kmh = 0.0f;
        g_vehicle_state.max_current_a = 0.0f;
        g_vehicle_state.range_km = 99.0f;
        g_vehicle_state.wh_per_km = 0.0f;
        g_vehicle_state.regen_active = false;
        g_vehicle_state.can_alive = false;  // Start as false, wait for frames
        g_vehicle_state.has_received_can = false;
        g_vehicle_state.last_can_rx_ms = 0;
        g_vehicle_state.speed_sample_count = 0;
        g_vehicle_state.speed_sum = 0.0f;
        g_vehicle_state.last_wh_update_ms = millis();
    }

    // Internal structures for Flipsky-specific telemetry
    struct EscData {
        float voltage = 0;
        float battery_current = 0;
        float motor_current = 0;
        float mosfet_temp = 0;
        float motor_temp = 0;
        float erpm = 0;
        float duty = 0;
        uint32_t last_update = 0;
    };

    static EscData master_esc;
    static EscData slave_esc;

    static int32_t parseI24(const uint8_t* data) {
        int32_t val = (data[0] << 16) | (data[1] << 8) | data[2];
        if (val & 0x800000) val |= 0xFF000000; // Sign extend from 24 to 32 bits
        return val;
    }

    static int16_t parseI16(const uint8_t* data) {
        return (int16_t)((data[0] << 8) | data[1]);
    }

    void poll() {
        uint32_t now = millis();

#ifdef ARDUINO
        esp_task_wdt_reset();
        // ============================================================
        // FLIPSKY FTESC CAN PROTOCOL (Custom EID format)
        // ============================================================
        twai_message_t message;
        while (twai_receive(&message, 0) == ESP_OK) {
            if (message.extd) {
                uint32_t raw_id = message.identifier;
                // Flipsky Layout: Bits 8-15 = ESC ID, Bits 0-7 = Command ID
                uint8_t mcu_id = (raw_id >> 8) & 0xFF;
                uint8_t cmd_id = raw_id & 0xFF;

                EscData* target = nullptr;
                if (mcu_id == 163) target = &master_esc;
                else if (mcu_id == 224) target = &slave_esc;

                if (target) {
                    target->last_update = now;

                    switch (cmd_id) {
                        case 0x0B: // Data 0: Currents (scaled by 1000)
                            target->motor_current = parseI24(&message.data[0]) / 1000.0f;
                            target->battery_current = parseI24(&message.data[3]) / 1000.0f;
                            break;
                        case 0x0C: // Data 1: ERPM & Duty
                            target->erpm = (float)parseI24(&message.data[0]);
                            target->duty = (float)parseI24(&message.data[3]) / 1000.0f;
                            break;
                        case 0x0D: // Data 2: Temps & Voltage (scaled by 100)
                            target->mosfet_temp = parseI16(&message.data[0]) / 100.0f;
                            target->motor_temp = parseI16(&message.data[2]) / 100.0f;
                            target->voltage = parseI16(&message.data[4]) / 100.0f;
                            break;
                    }
                }
            }
        }

        // ============================================================
        // DUAL ESC AGGREGATION Logic
        // ============================================================
        bool master_alive = (now - master_esc.last_update < 500);
        bool slave_alive = (now - slave_esc.last_update < 500);

        if (master_alive || slave_alive) {
            g_vehicle_state.can_alive = true;
            g_vehicle_state.has_received_can = true;
            g_vehicle_state.last_can_rx_ms = now;

            float v = 0, batt_amps = 0, mot_amps = 0;
            float max_fet = -100, max_mot = -100;
            int v_count = 0;

            if (master_alive) {
                v += master_esc.voltage;
                batt_amps += master_esc.battery_current;
                mot_amps += master_esc.motor_current;
                max_fet = master_esc.mosfet_temp;
                max_mot = master_esc.motor_temp;
                g_vehicle_state.erpm = (int32_t)master_esc.erpm;
                g_vehicle_state.duty_cycle = master_esc.duty;
                v_count++;
            }

            if (slave_alive) {
                v += slave_esc.voltage;
                batt_amps += slave_esc.battery_current;
                mot_amps += slave_esc.motor_current;
                if (slave_esc.mosfet_temp > max_fet) max_fet = slave_esc.mosfet_temp;
                if (slave_esc.motor_temp > max_mot) max_mot = slave_esc.motor_temp;
                // If master is dead, use slave erpm for speed
                if (!master_alive) {
                    g_vehicle_state.erpm = (int32_t)slave_esc.erpm;
                    g_vehicle_state.duty_cycle = slave_esc.duty;
                }
                v_count++;
            }

            if (v_count > 0) {
                g_vehicle_state.battery_voltage_v = v / v_count; // Average voltage
                g_vehicle_state.battery_current_a = batt_amps;  // Total current for Watts
                g_vehicle_state.current_a = mot_amps;           // Total motor amps
                g_vehicle_state.mosfet_temp_c = max_fet;
                g_vehicle_state.motor_temp_c = max_mot;
                g_vehicle_state.tachometer += (g_vehicle_state.erpm / 100); 

                // Minimal logging for confidence
                static uint32_t last_log = 0;
                if (now - last_log > 1000) {
                    Serial.printf("Flipsky Active: M_ID=163 (%.1fV, %.1fA) | S_ID=224 (%.1fV, %.1fA) | Sum=%.0fW\n", 
                        master_esc.voltage, master_esc.battery_current,
                        slave_esc.voltage, slave_esc.battery_current,
                        g_vehicle_state.power_w);
                    last_log = now;
                }
            }
        } else {
            g_vehicle_state.can_alive = false;
        }

#else
        // In Native Simulation, we now rely on the ReceiverApp sending mock TelemetryPackets
        // via esp_now_send which dash_onDataRecv will parse and write into g_vehicle_state.
        // We just need to ensure derived metrics don't overwrite the mocked values.
#endif

        // ============================================================
        // DERIVED METRICS (computed from raw data)
        // ============================================================

        // --- Power (Watts) ---
#ifdef ARDUINO
        g_vehicle_state.power_w = g_vehicle_state.battery_voltage_v * g_vehicle_state.battery_current_a;
#endif

        // --- Regen flag ---
        g_vehicle_state.regen_active = (g_vehicle_state.battery_current_a < -0.5f);

        // --- Speed for derived calcs ---
#ifdef ARDUINO
        float speed = calculate_speed_kmh(g_vehicle_state.erpm);
        g_vehicle_state.speed_kmh = speed;
#else
        float speed = g_vehicle_state.speed_kmh;
#endif

        // --- Top speed (session max) ---
        if (speed > g_vehicle_state.top_speed_kmh) {
            g_vehicle_state.top_speed_kmh = speed;
        }

        // --- Max motor current (session peak) ---
        float abs_current = fabs(g_vehicle_state.current_a);
        if (abs_current > g_vehicle_state.max_current_a) {
            g_vehicle_state.max_current_a = abs_current;
        }

        // --- Average speed (running) ---
        if (speed > 1.0f) {  // Only count when actually moving
            g_vehicle_state.speed_sum += speed;
            g_vehicle_state.speed_sample_count++;
            if (g_vehicle_state.speed_sample_count > 0) {
                g_vehicle_state.avg_speed_kmh = g_vehicle_state.speed_sum / (float)g_vehicle_state.speed_sample_count;
            }
        }

        // --- Wh consumed (integrate power over time) ---
        uint32_t dt_ms = now - g_vehicle_state.last_wh_update_ms;
        if (dt_ms > 0 && dt_ms < 1000) {  // Sanity guard
            float dt_h = (float)dt_ms / 3600000.0f;  // ms → hours
            float power = g_vehicle_state.power_w;
            // Only count positive power consumption (not regen recovery for simplicity)
            if (power > 0.0f) {
                g_vehicle_state.wh_consumed += power * dt_h;
            }
        }
        g_vehicle_state.last_wh_update_ms = now;

        // --- Wh/km efficiency ---
        float trip_km = ((float)g_vehicle_state.tachometer / 100000.0f);
        if (trip_km > 0.01f) {
            g_vehicle_state.wh_per_km = g_vehicle_state.wh_consumed / trip_km;
        }

        // --- Estimated range ---
        if (g_vehicle_state.wh_per_km > 1.0f && trip_km > 0.5f) {
            float batt_pct = ((g_vehicle_state.battery_voltage_v - 32.0f) / (42.0f - 32.0f));
            if (batt_pct < 0.0f) batt_pct = 0.0f;
            if (batt_pct > 1.0f) batt_pct = 1.0f;
            float remaining_wh = BATTERY_TOTAL_WH * batt_pct;
            g_vehicle_state.range_km = remaining_wh / g_vehicle_state.wh_per_km;
            if (g_vehicle_state.range_km > 99.0f) g_vehicle_state.range_km = 99.0f;
        } else {
            // Not enough data yet, estimate from battery % and a conservative Wh/km
            float batt_pct = ((g_vehicle_state.battery_voltage_v - 32.0f) / (42.0f - 32.0f));
            if (batt_pct < 0.0f) batt_pct = 0.0f;
            if (batt_pct > 1.0f) batt_pct = 1.0f;
            g_vehicle_state.range_km = (BATTERY_TOTAL_WH * batt_pct) / 25.0f;  // Assume ~25 Wh/km (conservative)
            if (g_vehicle_state.range_km > 99.0f) g_vehicle_state.range_km = 99.0f;
        }
    }

}
