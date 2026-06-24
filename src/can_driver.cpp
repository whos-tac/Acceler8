#include "can_driver.h"
#include "mechanical_config.h"
#include <math.h>
#include <stdint.h>
#include <string.h>
#include "odometer.h"

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

namespace CANDriver {

    void init() {
        memset(&g_vehicle_state, 0, sizeof(VehicleState));

#ifdef ARDUINO
        // Initialize ESP32 TWAI (CAN) hardware
        twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT((gpio_num_t)6, (gpio_num_t)0, TWAI_MODE_NORMAL);
        g_config.rx_queue_len = 1024; // HUGE queue to survive 8000Hz 0x00 spam without dropping telemetry!
        g_config.alerts_enabled = TWAI_ALERT_BUS_OFF | TWAI_ALERT_BUS_ERROR | TWAI_ALERT_RX_QUEUE_FULL;
        twai_timing_config_t t_config = TWAI_TIMING_CONFIG_1MBITS();
        twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

        if (twai_driver_install(&g_config, &t_config, &f_config) == ESP_OK) {
            twai_start();
        }
        esp_task_wdt_add(NULL);
        
        // Wait for ESC to finish its own boot sequence before latching IDs.
        // Without this, the dashboard grabs garbage IDs from ESC init frames.
        delay(1500);
        twai_message_t flush_msg;
        while (twai_receive(&flush_msg, 0) == ESP_OK) {} // drain queue
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

    static int32_t parseI32(const uint8_t* data) {
        return (int32_t)((data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3]);
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
        
        uint32_t alerts;
        if (twai_read_alerts(&alerts, 0) == ESP_OK) {
            // Alert logging removed to prevent USB CDC block
        }

        twai_message_t message;
        // Drain the queue completely on every loop to prevent overflow
        while (twai_receive(&message, 0) == ESP_OK) {
            if (message.extd && message.data_length_code >= 6) {
                uint32_t raw_id = message.identifier;
                // FLIPSKY FTESC EID Layout: Bits 0-7 = Command ID, Bits 8-15 = MCU ID (ESC ID)
                uint8_t command_id   = raw_id & 0xFF;
                uint8_t controller_id = (raw_id >> 8) & 0xFF;

                static int master_id = -1; // ponytail: latched on first valid Flipsky telemetry frame
                static int slave_id = -1;

                // Latch ESC IDs from the telemetry frames
                if (command_id == 0x0B || command_id == 0x0C || command_id == 0x0D) {
                    if (master_id == -1) {
                        master_id = controller_id;
                    } else if (slave_id == -1 && controller_id != master_id) {
                        slave_id = controller_id;
                    }
                }

                EscData* target = nullptr;
                if (controller_id == master_id) target = &master_esc;
                else if (controller_id == slave_id) target = &slave_esc;

                if (target) {
                    target->last_update = now;

                    switch (command_id) {
                        case 0x0B: // CAN_ESC_REALTIME_DATA_0: motor current*1000 (D0-D2), battery current*1000 (D3-D5), CRC (D6-D7)
                            target->motor_current   = (float)parseI24(&message.data[0]) / 1000.0f;
                            target->battery_current = (float)parseI24(&message.data[3]) / 1000.0f;
                            break;
                        case 0x0C: // CAN_ESC_REALTIME_DATA_1: ERPM (D0-D2), duty*1000 (D3-D5), CRC (D6-D7)
                            target->erpm  = fabs((float)parseI24(&message.data[0]));
                            target->duty  = (float)parseI24(&message.data[3]) / 1000.0f;
                            break;
                        case 0x0D: // CAN_ESC_REALTIME_DATA_2: mosfet_temp*100 (D0-D1), motor_temp*100 (D2-D3), voltage*100 (D4-D5), CRC (D6-D7)
                            target->mosfet_temp = (float)parseI16(&message.data[0]) / 100.0f;
                            target->motor_temp  = (float)parseI16(&message.data[2]) / 100.0f;
                            target->voltage     = (float)parseI16(&message.data[4]) / 100.0f;
                            break;
                    }
                }
            }
        }


        // ============================================================
        // DUAL ESC AGGREGATION Logic
        // ============================================================
        DASH_LOCK();
        
        bool master_alive = (master_esc.last_update != 0 && now - master_esc.last_update < 2500);
        bool slave_alive = (slave_esc.last_update != 0 && now - slave_esc.last_update < 2500);

        if (master_alive || slave_alive) {
            g_vehicle_state.can_alive = true;
            g_vehicle_state.has_received_can = true;
            g_vehicle_state.last_can_rx_ms = now;

            float v = 0, batt_amps = 0, mot_amps = 0;
            float max_fet = -100, max_mot = -100;
            int v_count = 0;

            int32_t erpm_m = master_alive ? abs((int32_t)master_esc.erpm) : 0;
            int32_t erpm_s = slave_alive ? abs((int32_t)slave_esc.erpm) : 0;
            g_vehicle_state.erpm = (erpm_m > erpm_s) ? erpm_m : erpm_s; // Use highest ERPM for instant response

            if (master_alive) {
                v += master_esc.voltage;
                batt_amps += master_esc.battery_current;
                mot_amps += master_esc.motor_current;
                max_fet = master_esc.mosfet_temp;
                max_mot = master_esc.motor_temp;
                g_vehicle_state.duty_cycle = master_esc.duty;
                v_count++;
            }

            if (slave_alive) {
                v += slave_esc.voltage;
                batt_amps += slave_esc.battery_current;
                mot_amps += slave_esc.motor_current;
                if (slave_esc.mosfet_temp > max_fet) max_fet = slave_esc.mosfet_temp;
                if (slave_esc.motor_temp > max_mot) max_mot = slave_esc.motor_temp;
                
                // If master is dead, duty cycle falls back to slave
                if (!master_alive) {
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

            }
        } else if (g_vehicle_state.has_received_can) {
            // NEVER TIMEOUT! If the ESC stops talking, keep the last known values!
            // This prevents "CAN Timeout" flashing when the VESC stops sending telemetry at 0 RPM.
        }

#else
        // In Native Simulation, we now rely on the ReceiverApp sending mock TelemetryPackets
        // via esp_now_send which dash_onDataRecv will parse and write into g_vehicle_state.
        // We just need to ensure derived metrics don't overwrite the mocked values.
#endif

        // ============================================================
        // DERIVED METRICS (computed from raw data)
        // ============================================================

        // --- Regen flag ---
        g_vehicle_state.regen_active = (g_vehicle_state.battery_current_a < -0.5f);

        // --- Speed & Power for derived calcs ---
#ifdef ARDUINO
        float speed;
        float raw_power;
        
        if (!g_vehicle_state.mock_mode_active) {
            float raw_speed = fabs(calculate_speed_kmh(g_vehicle_state.erpm));
            raw_power = g_vehicle_state.battery_voltage_v * g_vehicle_state.battery_current_a;
            
            // Apply Exponential Moving Average (EMA) to smooth out sporadic FTESC telemetry bursts!
            // If the FTESC drops frames or updates randomly, the UI will glide smoothly instead of jumping.
            if (g_vehicle_state.speed_kmh == 0 && raw_speed > 0.5f) {
                g_vehicle_state.speed_kmh = raw_speed; // Instant jump on first start
            } else {
                g_vehicle_state.speed_kmh = (g_vehicle_state.speed_kmh * 0.85f) + (raw_speed * 0.15f);
            }
            
            speed = g_vehicle_state.speed_kmh;
            
            g_vehicle_state.power_w = (g_vehicle_state.power_w * 0.7f) + (raw_power * 0.3f);
            g_vehicle_state.current_a = g_vehicle_state.battery_current_a;
        } else {
            speed = fabs(g_vehicle_state.speed_kmh);
        }
#else
        float speed = fabs(g_vehicle_state.speed_kmh);
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
        uint32_t dt_ms = now - g_vehicle_state.last_wh_update_ms;
        if (speed > 1.0f && dt_ms > 0 && dt_ms < 1000) {  // Only count when actually moving
            g_vehicle_state.speed_sum += (double)speed * dt_ms;
            g_vehicle_state.speed_sample_count += dt_ms;
            if (g_vehicle_state.speed_sample_count > 0) {
                g_vehicle_state.avg_speed_kmh = (float)(g_vehicle_state.speed_sum / (double)g_vehicle_state.speed_sample_count);
            }
        }

        // --- Wh consumed (integrate power over time) ---
        static double internal_wh_acc = 0.0;
        if (dt_ms > 0 && dt_ms < 1000) {  // Sanity guard
            double dt_h = (double)dt_ms / 3600000.0;  // ms → hours
            double power = (double)g_vehicle_state.power_w;
            // Only count positive power consumption (not regen recovery for simplicity)
            if (power > 0.0) {
                internal_wh_acc += power * dt_h;
                g_vehicle_state.wh_consumed = (float)internal_wh_acc;
            }
        }
        g_vehicle_state.last_wh_update_ms = now;

        // --- Wh/km efficiency ---
        float trip_km = (float)total_distance;
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
        
        DASH_UNLOCK();
    }

}
