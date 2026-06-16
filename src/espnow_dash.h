#pragma once
#include <stdint.h>

namespace EspNowDash {
    void init();
    void poll();
    void send_esc_config(bool settings_active, uint8_t gear, uint8_t direction, bool headlight_active);
}
