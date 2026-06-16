#pragma once
#include <stdint.h>

namespace EscUartDriver {
    /**
     * @brief Initialize UART port for ESC communication
     */
    void init();

    /**
     * @brief Sends a Flipsky UART packet to control throttle
     * @param throttle_val 0 to 1024, where 512 is neutral
     * @param gear Speed limit gear (0-4)
     * @param direction Motor rotation direction (0=Forward, 1=Reverse)
     * @param horn_active Boolean for horn state
     * @param headlight_active Boolean for headlight state
     * @param brake_light_active Boolean for brake light state
     */
    void send_throttle(uint16_t throttle_val, uint8_t gear = 0, uint8_t direction = 0, bool horn_active = false, bool headlight_active = false, bool brake_light_active = false);

    /**
     * @brief Sends a keep-alive packet to prevent UART timeout
     */
    void send_keep_alive();
}
