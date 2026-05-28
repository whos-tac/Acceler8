#pragma once

#include <stdint.h>
#ifdef ARDUINO
#include <Arduino.h>
#endif

namespace UnderglowController {
    /**
     * @brief Initialize the NeoPixelBus on GPIO 15.
     * Sets default colors (Purple) and clears the strip.
     */
    void init();

    /**
     * @brief Periodic update function. 
     * Handles animations (breathing/speed-reactive) based on global VehicleState.
     */
    void update();

    /**
     * @brief Simple helper to set a uniform color immediately.
     */
    void set_all(uint8_t r, uint8_t g, uint8_t b);
}
