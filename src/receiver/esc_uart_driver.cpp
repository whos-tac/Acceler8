#include "esc_uart_driver.h"

#ifdef ARDUINO
#include <Arduino.h>
#endif

// Flipsky FTESC UART Protocol V1.6 Constants
#define FTESC_STX                     0xAA
#define FTESC_ETX                     0xDD
#define FTESC_CMD_CONTROL             2      // 0x02 UART_CONTROL_AND_OBTAIN_DATA_ONCE
#define FTESC_CMD_KEEP_LIVE           0x19

namespace EscUartDriver {

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

    void init() {
#ifdef ARDUINO
        // Serial connected to ESC (or PC if in DEBUG_MODE)
        Serial.begin(115200);
#endif
    }

    void send_throttle(uint16_t throttle_val, uint8_t gear, uint8_t direction, bool horn_active, bool headlight_active, bool brake_light_active) {
#if defined(ARDUINO) && defined(RECEIVER_DEBUG_MODE)
        Serial.printf("[DEBUG] ESC Throttle Val: %u\n", throttle_val);
#elif defined(ARDUINO)
        uint8_t payload[15]; // 1 byte CMD + 14 bytes DATA
        payload[0] = FTESC_CMD_CONTROL; // 0x02
        payload[1] = 0x00;   // D0: Reserved
        payload[2] = 0x00;   // D1: Reserved
        payload[3] = (throttle_val >> 8) & 0xFF; // D2: Throttle High
        payload[4] = throttle_val & 0xFF;        // D3: Throttle Low
        payload[5] = 0x01;   // D4: Enable Direction Switching (Must be 1 for D5 to work)
        payload[6] = direction;   // D5: Motor Rotation Direction
        payload[7] = gear;   // D6: Speed Limit Gear
        payload[8] = horn_active ? 0x01 : 0x00;        // D7: Horn
        payload[9] = headlight_active ? 0x01 : 0x00;   // D8: Headlight
        payload[10] = brake_light_active ? 0x01 : 0x00; // D9: Brake Light
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
        Serial.write((crc >> 8) & 0xFF);  // High byte first (Big-endian)
        Serial.write(crc & 0xFF);         // Low byte second
        Serial.write(FTESC_ETX);
#else
        // Simulation - no UART output
        (void)throttle_val;
#endif
    }

    void send_keep_alive() {
#if defined(ARDUINO) && defined(RECEIVER_DEBUG_MODE)
        Serial.println("[DEBUG] ESC Keep-Alive (Heartbeat)");
#elif defined(ARDUINO)
        uint8_t payload[1] = { FTESC_CMD_KEEP_LIVE };
        uint16_t crc = calculateCrc16(payload, 1);
        
        Serial.write(FTESC_STX);
        Serial.write(1); // payload length (DLEN)
        Serial.write(payload, 1);
        Serial.write((crc >> 8) & 0xFF);  // High byte first
        Serial.write(crc & 0xFF);         // Low byte second
        Serial.write(FTESC_ETX);
#else
        // Simulation - no UART output
#endif
    }
}
