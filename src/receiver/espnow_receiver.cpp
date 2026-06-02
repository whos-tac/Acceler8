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
    static volatile uint32_t last_remote_rx_ms = 0;

    extern "C" void onDataRecv(uint8_t * mac, uint8_t *incomingData, uint8_t len) {
        if (memcmp(mac, remote_mac, 6) != 0) {
            return;
        }
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
        esp_now_add_peer(dash_mac, ESP_NOW_ROLE_COMBO, 0, NULL, 0);
#ifdef RECEIVER_DEBUG_MODE
        Serial.println("ESP-NOW initialized in COMBO mode.");
#endif
#endif
    }

    float get_latest_throttle() {
        return current_throttle;
    }

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

    void send_mock_telemetry(float speed, float battery_v, float power_w) {
        TelemetryPacket mock_pkt = {0};
        mock_pkt.speed_kmh = speed;
        mock_pkt.battery_voltage_v = battery_v;
        mock_pkt.power_w = power_w;
        mock_pkt.can_alive = true;
        
#ifdef ARDUINO
        esp_now_send(dash_mac, (uint8_t*)&mock_pkt, sizeof(TelemetryPacket));
#else
        esp_now_send(dash_mac, (uint8_t*)&mock_pkt, sizeof(TelemetryPacket));
#endif
    }
}
