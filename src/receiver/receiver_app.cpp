#include "receiver_app.h"
#ifdef ARDUINO
#include <Arduino.h>
#endif
#include "../espnow_packets.h"
#include "espnow_receiver.h"
#include "esc_uart_driver.h"
#include <cstring>
#include <cstdio>
#include <lvgl.h>

#ifndef ARDUINO
extern "C" uint32_t millis();
#endif

// ── Safety Configuration ──────────────────────────────────────────────
#define MAX_DRIVE_CURRENT_A   50.0f   // Max forward drive current (Amps)
#define MAX_BRAKE_CURRENT_A   20.0f   // Max regen brake current (Amps) — gentler than drive
#define THROTTLE_DEADZONE     3.0f    // ±3% deadzone at center
#define RAMP_RATE_PER_SEC     75.0f   // Max throttle change %/sec (0→100% in ~1.3s)
#define FAILSAFE_COAST_RATE   200.0f  // Throttle decay %/sec on signal loss (100→0% in ~0.5s)
#define FAILSAFE_TIMEOUT_MS   250     // ms before connection is considered lost
#define UART_UPDATE_MS        50      // UART command send interval (20Hz)

// Throttle state
static float ramped_throttle  = 0.0f;   // Smoothed output after ramp limiter

#ifndef ARDUINO
static lv_obj_t* lbl_receiver_throttle;
static lv_obj_t* slider_speed;
static lv_obj_t* slider_battery;
static lv_obj_t* slider_power;
#endif

void ReceiverApp::init() {
    EscUartDriver::init();
    EspnowReceiver::init();

#ifndef ARDUINO
    // Native LVGL setup
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x000000), 0);
    
    lbl_receiver_throttle = lv_label_create(lv_scr_act());
    lv_obj_set_style_text_color(lbl_receiver_throttle, lv_color_hex(0xFFFFFF), 0);
    lv_label_set_text(lbl_receiver_throttle, "RX: 0.0% -> 0 mA COAST");
    lv_obj_align(lbl_receiver_throttle, LV_ALIGN_TOP_LEFT, 10, 10);

    slider_speed = lv_slider_create(lv_scr_act());
    lv_slider_set_range(slider_speed, 0, 80);
    lv_slider_set_value(slider_speed, 0, LV_ANIM_OFF);
    lv_obj_set_size(slider_speed, 200, 20);
    lv_obj_align(slider_speed, LV_ALIGN_TOP_LEFT, 10, 50);
    lv_obj_t* lbl_spd = lv_label_create(lv_scr_act());
    lv_obj_set_style_text_color(lbl_spd, lv_color_hex(0xAAAAAA), 0);
    lv_label_set_text(lbl_spd, "Speed");
    lv_obj_align_to(lbl_spd, slider_speed, LV_ALIGN_OUT_RIGHT_MID, 10, 0);

    slider_battery = lv_slider_create(lv_scr_act());
    lv_slider_set_range(slider_battery, 30, 42);
    lv_slider_set_value(slider_battery, 42, LV_ANIM_OFF);
    lv_obj_set_size(slider_battery, 200, 20);
    lv_obj_align(slider_battery, LV_ALIGN_TOP_LEFT, 10, 100);
    lv_obj_t* lbl_batt = lv_label_create(lv_scr_act());
    lv_obj_set_style_text_color(lbl_batt, lv_color_hex(0xAAAAAA), 0);
    lv_label_set_text(lbl_batt, "Batt V");
    lv_obj_align_to(lbl_batt, slider_battery, LV_ALIGN_OUT_RIGHT_MID, 10, 0);

    slider_power = lv_slider_create(lv_scr_act());
    lv_slider_set_range(slider_power, -500, 2000);
    lv_slider_set_value(slider_power, 0, LV_ANIM_OFF);
    lv_obj_set_size(slider_power, 200, 20);
    lv_obj_align(slider_power, LV_ALIGN_TOP_LEFT, 10, 150);
    lv_obj_t* lbl_pwr = lv_label_create(lv_scr_act());
    lv_obj_set_style_text_color(lbl_pwr, lv_color_hex(0xAAAAAA), 0);
    lv_label_set_text(lbl_pwr, "Power W");
    lv_obj_align_to(lbl_pwr, slider_power, LV_ALIGN_OUT_RIGHT_MID, 10, 0);
#endif
}

void ReceiverApp::update() {
    static uint32_t last_loop_ms = 0;
    uint32_t now = millis();
    float dt = 0.0f;
    if (last_loop_ms != 0) {
        dt = (now - last_loop_ms) / 1000.0f;
    }
    last_loop_ms = now;

    // ── Determine target throttle and ramp rate ──
    uint32_t last_remote_rx_ms = EspnowReceiver::get_last_rx_ms();
    float current_throttle = EspnowReceiver::get_latest_throttle();

    bool signal_lost = (now - last_remote_rx_ms > FAILSAFE_TIMEOUT_MS);
    float target = signal_lost ? 0.0f : current_throttle;
    float rate   = signal_lost ? FAILSAFE_COAST_RATE : RAMP_RATE_PER_SEC;
    
    // ── Ramp limiter: smoothly move ramped_throttle towards target ──
    float max_delta = rate * dt; // max change per tick
    if (ramped_throttle < target) {
        ramped_throttle += max_delta;
        if (ramped_throttle > target) ramped_throttle = target;
    } else if (ramped_throttle > target) {
        ramped_throttle -= max_delta;
        if (ramped_throttle < target) ramped_throttle = target;
    }
    
    // ── Apply deadzone to output ──
    float output = ramped_throttle;
    if (output > -THROTTLE_DEADZONE && output < THROTTLE_DEADZONE) {
        output = 0.0f;
    }
    
    // ── Send to ESC every 50ms (20Hz) ──
    static uint32_t last_uart = 0;
    if (now - last_uart > UART_UPDATE_MS) {
        last_uart = now;
        
        // Map -100.0 -> 100.0 to 0 -> 1023 (512 is neutral)
        int32_t raw_val = 512 + (int32_t)(output * 5.12f);
        if (raw_val > 1023) raw_val = 1023;
        if (raw_val < 0) raw_val = 0;
        uint16_t throttle_val = (uint16_t)raw_val;
        
        EscUartDriver::send_throttle(throttle_val);

        // Send keep-alive command every 200ms to satisfy FTESC UART watchdog
        static uint32_t last_keep_alive = 0;
        if (now - last_keep_alive >= 200) {
            last_keep_alive = now;
            EscUartDriver::send_keep_alive();
        }

#if defined(DEBUG_ESPNOW) && !defined(RECEIVER_DEBUG_MODE)
#ifdef ARDUINO
        Serial.printf("[UART] Ramped: %.1f%% | Output: %.1f%% | Signal: %s\n",
                      ramped_throttle, output, signal_lost ? "LOST" : "OK");
#endif
#endif
    }

    // ── Send ReceiverStatusPacket to Dash every 100ms ──
    static uint32_t last_status_send = 0;
    if (now - last_status_send > 100) {
        last_status_send = now;
        EspnowReceiver::send_status_to_dash(signal_lost);
    }

#if defined(ARDUINO) && defined(RECEIVER_DEBUG_MODE)
    // Read from Serial to generate mock telemetry packet to Dashboard
    if (Serial.available()) {
        String line = Serial.readStringUntil('\n');
        line.trim();
        if (line.startsWith("telemetry speed ")) {
            float speed = line.substring(16).toFloat();
            EspnowReceiver::send_mock_telemetry(speed, 42.0f, 150.0f);
            Serial.printf("Sent mock telemetry: Speed %.1f\n", speed);
        }
    }
#elif !defined(ARDUINO)
    if (lbl_receiver_throttle) {
        char buf[64];
        int32_t raw_val = 512 + (int32_t)(output * 5.12f);
        if (raw_val > 1023) raw_val = 1023;
        if (raw_val < 0) raw_val = 0;
        snprintf(buf, sizeof(buf), "RX: %.1f%% -> THROTTLE: %d%s", 
                 ramped_throttle, (int)raw_val, signal_lost ? " [NO SIGNAL]" : "");
        lv_label_set_text(lbl_receiver_throttle, buf);
    }
    
    static uint32_t last_telemetry = 0;
    if (now - last_telemetry > 100) {
        last_telemetry = now;
        float s = slider_speed ? lv_slider_get_value(slider_speed) : 0;
        float v = slider_battery ? lv_slider_get_value(slider_battery) : 42.0;
        float p = slider_power ? lv_slider_get_value(slider_power) : 0;
        EspnowReceiver::send_mock_telemetry(s, v, p);
    }
#endif
}
