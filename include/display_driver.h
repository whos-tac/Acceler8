#pragma once
#include <Arduino.h>

// LVGL needs to be included for display interfacing
// #include <lvgl.h> 

namespace DisplayDriver {
    /**
     * @brief Initialize the ST7701 Display, GT911 Touch Controller, and LVGL.
     * Uses hardware pins defined in hardware_pinout.md.
     */
    void init();

    /**
     * @brief Must be called periodically to pump the LVGL task handler.
     */
    void tick();
}
