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
     */
    void send_throttle(uint16_t throttle_val);

    /**
     * @brief Sends a keep-alive packet to prevent UART timeout
     */
    void send_keep_alive();
}
