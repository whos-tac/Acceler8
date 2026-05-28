#pragma once

#include <stdint.h>

namespace SettingsScreen {
    void init();
    void update(uint8_t current_btn_state);
    void enter();
    bool is_active();
}
