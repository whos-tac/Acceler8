#include "espnow_receiver.h"
#include "../espnow_packets.h"
#include <cstring>
#include <cstdio>

#ifdef ARDUINO
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <espnow.h>
#include <user_interface.h>
#else
// Mock ESP-NOW & Serial
extern "C" uint32_t millis();
extern "C" void esp_now_send(const uint8_t *peer_addr, const uint8_t *data, size_t len);
#endif

// MAC Addresses
static uint8_t remote_mac[] = {0xD0, 0xCF, 0x13, 0x32, 0x42, 0x3C};
static uint8_t dash_mac[]   = {0x3C, 0x0F, 0x02, 0xC2, 0xD4, 0xCC};

namespace EspnowReceiver {

    static volatile float current_throttle = 0.0f;
    static volatile uint8_t current_button_state = 0;
    static volatile uint32_t last_remote_rx_ms = 0;

    static volatile bool current_settings_active = false;
    static volatile uint8_t current_gear = 1;
    static volatile uint8_t current_direction = 0;
    static volatile bool current_headlight_active = false;

    extern "C" void onDataRecv(uint8_t * mac, uint8_t *incomingData, uint8_t len) {
        if (memcmp(mac, remote_mac, 6) == 0) {
            if (len == sizeof(ControlPacket)) {
                ControlPacket pkt;
                memcpy(&pkt, incomingData, sizeof(ControlPacket));
                
                // Clamp to valid range (protects against corrupted/spoofed packets)
                float t = pkt.throttle_percent;
                if (t > 100.0f) t = 100.0f;
                if (t < -100.0f) t = -100.0f;
                current_throttle = t;
                current_button_state = pkt.button_state;
                
                last_remote_rx_ms = millis();

#if defined(DEBUG_ESPNOW) && defined(RECEIVER_DEBUG_MODE)
#ifdef ARDUINO
                Serial.printf("[ESP-NOW] RX Remote -> Receiver | Throttle: %.1f%%\n", current_throttle);
#endif
#endif
            }
        } else if (memcmp(mac, dash_mac, 6) == 0) {
            if (len == sizeof(EscConfigPacket)) {
                EscConfigPacket pkt;
                memcpy(&pkt, incomingData, sizeof(EscConfigPacket));
                current_settings_active = pkt.settings_active;
                current_gear = pkt.gear;
                current_direction = pkt.direction;
                current_headlight_active = pkt.headlight_active;
            }
        }
    }

    void init() {
#ifdef ARDUINO
        WiFi.disconnect(true);
        WiFi.mode(WIFI_STA);
        wifi_set_channel(1);
        if (esp_now_init() != 0) {
            return;
        }
        esp_now_set_self_role(ESP_NOW_ROLE_COMBO);
        esp_now_register_recv_cb(onDataRecv);

        // Add Dashboard as a peer so we can send telemetry back
        esp_now_add_peer(dash_mac, ESP_NOW_ROLE_COMBO, 1, NULL, 0);
#if defined(DEBUG_ESPNOW) && defined(RECEIVER_DEBUG_MODE)
        Serial.println("ESP-NOW initialized in COMBO mode.");
#endif
#endif
    }

    float get_latest_throttle() {
        return current_throttle;
    }

    uint8_t get_latest_button_state() {
        return current_button_state;
    }

    bool is_settings_active() { return current_settings_active; }
    uint8_t get_gear() { return current_gear; }
    uint8_t get_direction() { return current_direction; }
    bool is_headlight_active() { return current_headlight_active; }

    uint32_t get_last_rx_ms() {
        return last_remote_rx_ms;
    }

    void send_status_to_dash(bool signal_lost) {
#ifdef ARDUINO
        ReceiverStatusPacket status_pkt;
        status_pkt.remote_disconnected = signal_lost;
        esp_now_send(dash_mac, (uint8_t*)&status_pkt, sizeof(ReceiverStatusPacket));
#else
        ReceiverStatusPacket status_pkt;
        status_pkt.remote_disconnected = signal_lost;
        esp_now_send(dash_mac, (uint8_t*)&status_pkt, sizeof(ReceiverStatusPacket));
#endif
    }

    void send_mock_telemetry(float speed, float battery_v, float power_w, float current_a) {
        TelemetryPacket mock_pkt = {0};
        mock_pkt.speed_kmh = speed;
        mock_pkt.battery_voltage_v = battery_v;
        mock_pkt.battery_current_a = current_a;
        mock_pkt.power_w = power_w;
        mock_pkt.can_alive = true;
        
        esp_now_send(dash_mac, (uint8_t*)&mock_pkt, sizeof(TelemetryPacket));
    }
}
