#pragma once
#ifdef ARDUINO
#include <Arduino.h>
#endif
#include "can_driver.h"

namespace UIController {
    /**
     * @brief Set up the foundational LVGL layouts based on ui_specifications.md.
     */
    void init();

    extern uint8_t global_last_btn;
    extern bool touch_enabled;

    /**
     * @brief Updates the labels and UI elements with new values from the global VehicleState.
     */
    void update();
    void show_main_screen();
}
