#include "receiver_app.h"
#ifdef ARDUINO
#include <Arduino.h>
#endif
#include "../espnow_packets.h"
#include <cstring>
#include <cstdio>
#include <lvgl.h>

#ifdef ARDUINO
#include <ESP8266WiFi.h>
#include <espnow.h>
#else
// Mock ESP-NOW & Serial
extern "C" uint32_t millis();
extern "C" void esp_now_send(const uint8_t *peer_addr, const uint8_t *data, size_t len);
#endif

// ── Safety Configuration ──────────────────────────────────────────────
#define MAX_DRIVE_CURRENT_A   50.0f   // Max forward drive current (Amps)
#define MAX_BRAKE_CURRENT_A   20.0f   // Max regen brake current (Amps) — gentler than drive
#define THROTTLE_DEADZONE     3.0f    // ±3% deadzone at center
#define RAMP_RATE_PER_SEC     75.0f   // Max throttle change %/sec (0→100% in ~1.3s)
#define FAILSAFE_COAST_RATE   200.0f  // Throttle decay %/sec on signal loss (100→0% in ~0.5s)
#define FAILSAFE_TIMEOUT_MS   250     // ms before connection is considered lost
#define UART_UPDATE_MS        50      // UART command send interval (20Hz)

// Flipsky FTESC UART Protocol V1.6 Constants
#define FTESC_STX                     0xAA
#define FTESC_ETX                     0xDD
#define FTESC_CMD_CONTROL             2      // 0x02 UART_CONTROL_AND_OBTAIN_DATA_ONCE
#define FTESC_CMD_KEEP_LIVE           0x19

// MAC Addresses
static uint8_t remote_mac[] = {0xD0, 0xCF, 0x13, 0x32, 0x42, 0x3C};
static uint8_t dash_mac[]   = {0x3C, 0x0F, 0x02, 0xC2, 0xD4, 0xCC};

// Throttle state
static float current_throttle = 0.0f;   // Raw input from remote (jumps instantly)
static float ramped_throttle  = 0.0f;   // Smoothed output after ramp limiter
static uint32_t last_remote_rx_ms = 0;

#ifndef ARDUINO
static lv_obj_t* lbl_receiver_throttle;
static lv_obj_t* slider_speed;
static lv_obj_t* slider_battery;
static lv_obj_t* slider_power;
#endif

// Standard Modbus CRC-16 for Flipsky FTESC
static uint16_t calculateCrc16(const uint8_t *buf, uint32_t len) {
    uint16_t crc = 0xFFFF;
    for (uint32_t i = 0; i < len; i++) {
        crc ^= buf[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 1) {
                crc = (crc >> 1) ^ 0xA001;
            } else {
                crc >>= 1;
            }
        }
    }
    return crc;
}

// ── Generic FTESC UART Command Sender ──────────────────────────────────
// Sends a Flipsky UART packet for UART Mode: [0xAA] [15] [payload] [CRC16] [0xDD]
// Used for UART_CONTROL_AND_OBTAIN_DATA_ONCE (ID 2)
static void sendFlipskyUartModeCommand(uint16_t throttle_val) {
#if defined(ARDUINO) && defined(RECEIVER_DEBUG_MODE)
    Serial.printf("[DEBUG] ESC Throttle Val: %u\n", throttle_val);
#elif defined(ARDUINO)
    uint8_t payload[15]; // 1 byte CMD + 14 bytes DATA
    payload[0] = FTESC_CMD_CONTROL; // 0x02
    payload[1] = 0x00;   // D0: Reserved
    payload[2] = 0x00;   // D1: Reserved
    payload[3] = (throttle_val >> 8) & 0xFF; // D2: Throttle High
    payload[4] = throttle_val & 0xFF;        // D3: Throttle Low
    payload[5] = 0x00;   // D4: Enable Direction Switching
    payload[6] = 0x00;   // D5: Motor Rotation Direction
    payload[7] = 0x00;   // D6: Speed Limit Gear
    payload[8] = 0x00;   // D7: Horn
    payload[9] = 0x00;   // D8: Headlight
    payload[10] = 0x00;  // D9: Brake Light
    payload[11] = 0x00;  // D10: Enable Cruise
    payload[12] = 0x00;  // D11: Cruise Mode
    payload[13] = 0x00;  // D12: Enable Multi-mode
    payload[14] = 0x00;  // D13: Multi-mode switching
    
    // Calculate Modbus CRC16
    uint16_t crc = calculateCrc16(payload, 15);
    
    // Send packet
    Serial.write(FTESC_STX);
    Serial.write(15); // payload length (DLEN)
    Serial.write(payload, 15);
    Serial.write((crc >> 8) & 0xFF);
    Serial.write(crc & 0xFF);
    Serial.write(FTESC_ETX);
#else
    // Simulation — no UART output
    (void)throttle_val;
#endif
}

static void sendFlipskyKeepAlive() {
#if defined(ARDUINO) && defined(RECEIVER_DEBUG_MODE)
    Serial.println("[DEBUG] ESC Keep-Alive (Heartbeat)");
#elif defined(ARDUINO)
    uint8_t payload[1] = { FTESC_CMD_KEEP_LIVE };
    uint16_t crc = calculateCrc16(payload, 1);
    
    Serial.write(FTESC_STX);
    Serial.write(1); // payload length (DLEN)
    Serial.write(payload, 1);
    Serial.write((crc >> 8) & 0xFF);
    Serial.write(crc & 0xFF);
    Serial.write(FTESC_ETX);
#else
    // Simulation — no UART output
#endif
}

extern "C" void receiver_onDataRecv(uint8_t * mac, uint8_t *incomingData, uint8_t len) {
    if (len == sizeof(ControlPacket)) {
        ControlPacket pkt;
        memcpy(&pkt, incomingData, sizeof(ControlPacket));
        
        // Clamp to valid range (protects against corrupted/spoofed packets)
        float t = pkt.throttle_percent;
        if (t > 100.0f) t = 100.0f;
        if (t < -100.0f) t = -100.0f;
        current_throttle = t;
        
        last_remote_rx_ms = millis();

#ifdef DEBUG_ESPNOW
#ifdef ARDUINO
        Serial.printf("[ESP-NOW] RX Remote -> Receiver | Throttle: %.1f%%\n", current_throttle);
#else
        // printf("[ESP-NOW] RX Remote -> Receiver | Throttle: %.1f%%\n", current_throttle);
#endif
#endif
    }
}

void ReceiverApp::init() {
#ifdef ARDUINO
    // Serial connected to ESC (or PC if in DEBUG_MODE)
    Serial.begin(115200);

    WiFi.mode(WIFI_STA);
    if (esp_now_init() != 0) {
        return;
    }
#ifdef RECEIVER_DEBUG_MODE
    esp_now_set_self_role(ESP_NOW_ROLE_COMBO);
#else
    esp_now_set_self_role(ESP_NOW_ROLE_SLAVE);
#endif
    esp_now_register_recv_cb(receiver_onDataRecv);

#ifdef RECEIVER_DEBUG_MODE
    // Add Dashboard as a peer so we can send debug telemetry back
    esp_now_add_peer(dash_mac, ESP_NOW_ROLE_COMBO, 0, NULL, 0);
    Serial.println("Receiver SAFE MODE initialized (FTESC UART Mode).");
    Serial.printf("  Ramp: %.0f%%/s\n", RAMP_RATE_PER_SEC);
    Serial.println("Enter 'telemetry speed <val>' to mock ESC.");
#endif

#else
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
    // ── Determine target throttle and ramp rate ──
    bool signal_lost = (millis() - last_remote_rx_ms > FAILSAFE_TIMEOUT_MS);
    float target = signal_lost ? 0.0f : current_throttle;
    float rate   = signal_lost ? FAILSAFE_COAST_RATE : RAMP_RATE_PER_SEC;
    
    // ── Ramp limiter: smoothly move ramped_throttle towards target ──
    float max_delta = rate * (UART_UPDATE_MS / 1000.0f); // max change per tick
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
    if (millis() - last_uart > UART_UPDATE_MS) {
        last_uart = millis();
        
        // Map -100.0 -> 100.0 to 0 -> 1023 (512 is neutral)
        int32_t raw_val = 512 + (int32_t)(output * 5.12f);
        if (raw_val > 1023) raw_val = 1023;
        if (raw_val < 0) raw_val = 0;
        uint16_t throttle_val = (uint16_t)raw_val;
        
        sendFlipskyUartModeCommand(throttle_val);

        // Send keep-alive command every 200ms to satisfy FTESC UART watchdog
        static uint32_t last_keep_alive = 0;
        if (millis() - last_keep_alive >= 200) {
            last_keep_alive = millis();
            sendFlipskyKeepAlive();
        }

#if defined(DEBUG_ESPNOW) && !defined(RECEIVER_DEBUG_MODE)
#ifdef ARDUINO
        Serial.printf("[UART] Ramped: %.1f%% | Output: %.1f%% | Signal: %s\n",
                      ramped_throttle, output, signal_lost ? "LOST" : "OK");
#else
        // printf("[UART] Ramped: %.1f%% | Output: %.1f%% | Signal: %s\n",
        //        ramped_throttle, output, signal_lost ? "LOST" : "OK");
#endif
#endif
    }

    // ── Send ReceiverStatusPacket to Dash every 100ms ──
    static uint32_t last_status_send = 0;
    if (millis() - last_status_send > 100) {
        last_status_send = millis();
        ReceiverStatusPacket status_pkt;
        status_pkt.remote_disconnected = signal_lost;
        esp_now_send(dash_mac, (uint8_t*)&status_pkt, sizeof(ReceiverStatusPacket));
    }

#if defined(ARDUINO) && defined(RECEIVER_DEBUG_MODE)
    // Read from Serial to generate mock telemetry packet to Dashboard
    if (Serial.available()) {
        String line = Serial.readStringUntil('\n');
        line.trim();
        if (line.startsWith("telemetry speed ")) {
            float speed = line.substring(16).toFloat();
            TelemetryPacket mock_pkt = {0};
            mock_pkt.speed_kmh = speed;
            mock_pkt.battery_voltage_v = 42.0f;
            mock_pkt.power_w = 150.0f;
            
            esp_now_send(dash_mac, (uint8_t*)&mock_pkt, sizeof(TelemetryPacket));
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
    if (millis() - last_telemetry > 100) {
        last_telemetry = millis();
        TelemetryPacket mock_pkt = {0};
        if (slider_speed) mock_pkt.speed_kmh = lv_slider_get_value(slider_speed);
        if (slider_battery) mock_pkt.battery_voltage_v = lv_slider_get_value(slider_battery);
        if (slider_power) mock_pkt.power_w = lv_slider_get_value(slider_power);
        mock_pkt.can_alive = true;
        
        esp_now_send(dash_mac, (uint8_t*)&mock_pkt, sizeof(TelemetryPacket));
    }
#endif
}
