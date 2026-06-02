#pragma once
#include <stdint.h>

// Shared ESP-NOW packets

// Packet sent from Dash to Remote
typedef struct {
    float speed_kmh;
    float battery_voltage_v;
    float battery_current_a;
    float power_w;
    float motor_temp_c;
    float mosfet_temp_c;
    float range_km;
    bool can_alive;
    bool remote_disconnected;
} __attribute__((packed)) TelemetryPacket;

// Packet sent from Remote to Receiver and Dash
typedef struct {
    float throttle_percent; // -100.0 (full brake) to 100.0 (full throttle)
    uint8_t button_state;   // Bitmask: bit 0=UP, 1=DOWN, 2=LEFT, 3=RIGHT, 4=CONFIRM
} __attribute__((packed)) ControlPacket;

// Packet sent from Receiver to Dash
typedef struct {
    bool remote_disconnected;
} __attribute__((packed)) ReceiverStatusPacket;

// MAC Addresses:
// Dash: 3C:0F:02:C2:D4:CC
// Remote: D0:CF:13:32:42:3C
// Receiver: EC:64:C9:CC:D8:54
