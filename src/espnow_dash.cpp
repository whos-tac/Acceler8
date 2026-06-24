#include "espnow_dash.h"
#include "espnow_packets.h"
#include "can_driver.h"
#include "settings_screen.h"
#include <cstring>

#ifdef ARDUINO
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#else
typedef uint8_t esp_now_send_status_t;
#define ESP_NOW_SEND_SUCCESS 0
extern "C" uint32_t millis();
extern "C" void esp_now_send(const uint8_t *peer_addr, const uint8_t *data, size_t len);
#endif

// Remote MAC Address: D0:CF:13:32:42:3C
static uint8_t remote_mac[] = {0xD0, 0xCF, 0x13, 0x32, 0x42, 0x3C};
// Receiver MAC Address (for debug): EC:64:C9:CC:D8:54
static uint8_t receiver_mac[] = {0xEC, 0x64, 0xC9, 0xCC, 0xD8, 0x54};

static uint32_t last_send_time = 0;
// Global for Debug UI screen
extern bool debug_mode_active;

static void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
#ifdef DEBUG_ESPNOW
#ifdef ARDUINO
    if (status != ESP_NOW_SEND_SUCCESS) {
        Serial.println("[ESP-NOW] TX Dash -> Remote FAILED");
    }
#else
    if (status != ESP_NOW_SEND_SUCCESS) {
        printf("[ESP-NOW] TX Dash -> Remote FAILED\n");
    }
#endif
#endif
}

extern "C" void dash_onDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
    if (len == sizeof(TelemetryPacket)) {
        TelemetryPacket pkt;
        memcpy(&pkt, incomingData, sizeof(TelemetryPacket));
        
        // When receiving telemetry over ESP-NOW, it's debug data from Receiver mock.
        // Update vehicle state so UI reflects it.
        DASH_LOCK();
        g_vehicle_state.mock_mode_active = true;
        g_vehicle_state.speed_kmh = pkt.speed_kmh;
        g_vehicle_state.erpm = pkt.speed_kmh * 100.0f; // mock erpm
        g_vehicle_state.battery_voltage_v = pkt.battery_voltage_v;
        g_vehicle_state.battery_current_a = pkt.battery_current_a;
        g_vehicle_state.power_w = pkt.power_w;
        g_vehicle_state.can_alive = true; // fake CAN alive
        g_vehicle_state.has_received_can = true;
        DASH_UNLOCK();

#ifdef DEBUG_ESPNOW
#ifdef ARDUINO
        Serial.printf("[ESP-NOW] RX Receiver -> Dash | Speed: %.1f km/h\n", pkt.speed_kmh);
#else
        printf("[ESP-NOW] RX Receiver -> Dash | Speed: %.1f km/h\n", pkt.speed_kmh);
#endif
#endif
    } else if (len == sizeof(ReceiverStatusPacket)) {
        ReceiverStatusPacket pkt;
        memcpy(&pkt, incomingData, sizeof(ReceiverStatusPacket));
        DASH_LOCK();
        g_vehicle_state.remote_disconnected = pkt.remote_disconnected;
        DASH_UNLOCK();
        
#ifdef DEBUG_ESPNOW
#ifdef ARDUINO
        Serial.printf("[ESP-NOW] RX Receiver -> Dash | Remote Disconnected: %d\n", pkt.remote_disconnected);
#else
        printf("[ESP-NOW] RX Receiver -> Dash | Remote Disconnected: %d\n", pkt.remote_disconnected);
#endif
#endif
    } else if (len == sizeof(ControlPacket)) {
        ControlPacket pkt;
        memcpy(&pkt, incomingData, sizeof(ControlPacket));
        DASH_LOCK();
        g_vehicle_state.remote_button_state = pkt.button_state;
        g_vehicle_state.remote_throttle = pkt.throttle_percent;
        DASH_UNLOCK();
    }
}

namespace EspNowDash {

    void init() {
#ifdef ARDUINO
        WiFi.disconnect(true);
        // Set WiFi to station mode
        WiFi.mode(WIFI_STA);
        
        // Spoof the MAC address to match the old dashboard
        uint8_t dash_mac[] = {0x3C, 0x0F, 0x02, 0xC2, 0xD4, 0xCC};
        esp_wifi_set_mac(WIFI_IF_STA, dash_mac);

        esp_wifi_set_promiscuous(true);
        esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);
        esp_wifi_set_promiscuous(false);

        // Initialize ESP-NOW
        if (esp_now_init() != ESP_OK) {
            Serial.println("Error initializing ESP-NOW");
            return;
        }

        // Register callbacks
        esp_now_register_send_cb(onDataSent);
        esp_now_register_recv_cb((esp_now_recv_cb_t)dash_onDataRecv);

        // Register peer (Remote)
        static esp_now_peer_info_t peerInfo;
        memcpy(peerInfo.peer_addr, remote_mac, 6);
        peerInfo.channel = 1;  
        peerInfo.encrypt = false;
        
        if (esp_now_add_peer(&peerInfo) != ESP_OK) {
            Serial.println("Failed to add Remote peer");
            return;
        }

        // Register peer (Receiver for debug)
        static esp_now_peer_info_t rxPeerInfo;
        memcpy(rxPeerInfo.peer_addr, receiver_mac, 6);
        rxPeerInfo.channel = 1;
        rxPeerInfo.encrypt = false;
        esp_now_add_peer(&rxPeerInfo);
#endif
    }

    void poll() {
        // Send telemetry every 100ms (10Hz)
        if (millis() - last_send_time > 100) {
            last_send_time = millis();

            TelemetryPacket packet = {0};
            DASH_LOCK();
            packet.speed_kmh = g_vehicle_state.speed_kmh;
            packet.battery_voltage_v = g_vehicle_state.battery_voltage_v;
            packet.battery_current_a = g_vehicle_state.battery_current_a;
            packet.power_w = g_vehicle_state.power_w;
            packet.motor_temp_c = g_vehicle_state.motor_temp_c;
            packet.mosfet_temp_c = g_vehicle_state.mosfet_temp_c;
            packet.range_km = g_vehicle_state.range_km;
            packet.can_alive = g_vehicle_state.can_alive;
            DASH_UNLOCK();

            esp_now_send(remote_mac, (uint8_t *)&packet, sizeof(TelemetryPacket));

            // Also send EscConfigPacket to the receiver!
            send_esc_config(SettingsScreen::is_active(), SettingsScreen::esc_gear, SettingsScreen::esc_direction, SettingsScreen::headlight_active);
        }
    }

    void send_esc_config(bool settings_active, uint8_t gear, uint8_t direction, bool headlight_active) {
        EscConfigPacket packet;
        packet.settings_active = settings_active;
        packet.gear = gear;
        packet.direction = direction;
        packet.headlight_active = headlight_active;
        esp_now_send(receiver_mac, (uint8_t *)&packet, sizeof(EscConfigPacket));
    }

}
