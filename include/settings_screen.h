#pragma once

#include <stdint.h>

namespace SettingsScreen {
    void init();
    void update(uint8_t current_btn_state);
    void enter();
    bool is_active();

    extern uint8_t esc_gear;
    extern uint8_t esc_direction;
    extern uint8_t display_brightness;
    extern uint16_t underglow_hue;
    extern bool headlight_active;
}
