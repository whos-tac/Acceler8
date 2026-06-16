// UART ESC Diagnostic for Flipsky FTESC — tests commands directly, no ESP-NOW
// Sends a small motor current command and blinks LED to show activity
// Connect D1 Mini TX->ESC RX, RX->ESC TX, GND->GND

#include <Arduino.h>

#define LED_PIN 2  // D1 Mini built-in LED (active LOW)

// Flipsky FTESC UART Protocol V1.6 Constants
#define FTESC_STX                     0xAA
#define FTESC_ETX                     0xDD
#define FTESC_CMD_CONTROL             2      // 0x02 UART_CONTROL_AND_OBTAIN_DATA_ONCE
#define FTESC_CMD_KEEP_LIVE           0x19

// ── TEST CONFIGURATION ──
#define TEST_THROTTLE 600      // 512 is neutral, >512 drive, <512 brake. 600 = gentle drive

// Standard Modbus CRC-16
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

static void sendFlipskyUartModeCommand(uint16_t throttle_val, bool horn, bool headlight, bool brake) {
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
    
    uint16_t crc = calculateCrc16(payload, 15);
    
    Serial.write(FTESC_STX);       // Start byte 0xAA
    Serial.write((uint8_t)15);     // DLEN
    Serial.write(payload, 15);     // Payload (CMD + DATA)
    Serial.write((uint8_t)((crc >> 8) & 0xFF)); // CRC high
    Serial.write((uint8_t)(crc & 0xFF));         // CRC low
    Serial.write(FTESC_ETX);       // End byte 0xDD
}

static void sendKeepAlive() {
    uint8_t payload[1] = { FTESC_CMD_KEEP_LIVE };
    uint16_t crc = calculateCrc16(payload, 1);
    
    Serial.write(FTESC_STX);       // Start byte 0xAA
    Serial.write((uint8_t)1);      // DLEN (INFO length = 1 byte CMD)
    Serial.write(payload, 1);      // Payload (CMD)
    Serial.write((uint8_t)((crc >> 8) & 0xFF)); // CRC high (Big-Endian)
    Serial.write((uint8_t)(crc & 0xFF));         // CRC low (Big-Endian)
    Serial.write(FTESC_ETX);       // End byte 0xDD
}

void setup() {
    Serial.begin(115200);
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, HIGH); // LED off (active LOW)
    
    // 3 fast blinks to show we booted
    for (int i = 0; i < 3; i++) {
        digitalWrite(LED_PIN, LOW);
        delay(100);
        digitalWrite(LED_PIN, HIGH);
        delay(100);
    }
    
    delay(2000); // Wait 2s for ESC to be ready
}

void loop() {
    // Send test command every 50ms (20Hz)
    sendFlipskyUartModeCommand(TEST_THROTTLE, false, false, false);
    
    // Send keep-alive command every 200ms (every 4th iteration of the 50ms loop)
    static uint32_t last_keep_alive = 0;
    if (millis() - last_keep_alive >= 200) {
        last_keep_alive = millis();
        sendKeepAlive();
    }
    
    // Toggle LED on every send (visible blink = UART is transmitting)
    static bool led_state = false;
    led_state = !led_state;
    digitalWrite(LED_PIN, led_state ? LOW : HIGH);
    
    delay(50);
}
