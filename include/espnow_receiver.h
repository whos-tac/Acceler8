#pragma once
#include <stdint.h>

namespace EspnowReceiver {
    /**
     * @brief Initialize ESP-NOW networking for the receiver
     */
    void init();

    /**
     * @brief Get the latest parsed throttle target received from remote
     * @return Throttle percent (-100.0 to 100.0)
     */
    float get_latest_throttle();

    /**
     * @brief Get the latest button state received from remote
     * @return Button state bitmask
     */
    uint8_t get_latest_button_state();

    bool is_settings_active();
    uint8_t get_gear();
    uint8_t get_direction();
    bool is_headlight_active();

    /**
     * @brief Get the timestamp of the last valid control packet received
     * @return Timestamp in ms
     */
    uint32_t get_last_rx_ms();

    /**
     * @brief Send status packet back to Dash
     * @param signal_lost true if the receiver considers the remote disconnected
     */
    void send_status_to_dash(bool signal_lost);

    /**
     * @brief Send a mock telemetry packet back to Dash (used in Debug modes)
     * @param speed Simulated speed
     * @param battery_v Simulated battery
     * @param power_w Simulated power
     * @param current_a Simulated battery current
     */
    void send_mock_telemetry(float speed, float battery_v, float power_w, float current_a);
}
