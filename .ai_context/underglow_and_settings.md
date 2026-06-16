# Underglow & Settings Architecture

This document covers two interactive auxiliary systems in the ACCELER8 DashBoard: the **Settings Screen** and the **Underglow Controller**. Both systems allow dynamic reconfiguration without flashing new firmware.

## 1. Settings Screen (`settings_screen.cpp`)
The Settings Screen is an LVGL-based modal interface that pauses the main dashboard to allow parameter editing.

### Access & Navigation
*   **Trigger**: Holding **UP + DOWN** on the remote D-pad for 2 seconds enters the Settings Screen.
*   **Input**: Driven by the `remote_button_state` (Up, Down, Left, Right, Confirm). Note that **CONFIRM** is mapped to the horn during normal riding, but used as "Enter" inside menus.
*   **State Machine**:
    *   `SETTING_MODE_MENU`: Navigating between configurable fields.
    *   `SETTING_MODE_EDIT`: Actively modifying a selected field's value (D-pad UP/DOWN/LEFT/RIGHT adjusts values).

### Configurable Variables (D-Pad)
1.  **Pole Pairs (`motor_pole_pairs`)**: Required for correct ERPM to RPM conversion.
2.  **Gear Ratio (`gear_ratio`)**: Required for RPM to wheel rotation conversion.
3.  **Wheel Diameter (`wheel_diameter_mm`)**: Required for wheel rotation to linear speed.
4.  **Odometer**: Displays total distance. Triggering 'Confirm' while in edit mode resets the `total_distance` accumulator in `odometer.cpp`.

### Touch Modal Settings (GT911)
Selecting "Touch Settings" from the menu opens a touch-enabled modal. Touch interactions are exclusively active here to prevent accidental inputs while riding.
1.  **ESC Gear**: Dropdown for None, Low, Medium, High.
2.  **ESC Direction**: Dropdown for Forward, Reverse.
3.  **Headlight**: Toggle switch for front headlights.
4.  **Brightness**: Slider for display backlight (10-100%).
5.  **Underglow Color**: LVGL Colorwheel to select `underglow_hue`.

### Persistence
When the user selects **Save & Exit**, the UI Controller calls `save_mechanical_config()`, which commits the updated parameters (Pole Pairs, Gear Ratio, Wheel Diameter) to ESP32 Non-Volatile Storage (NVS) via the `Preferences` library. On boot, `load_mechanical_config()` restores these values.

---

## 2. Underglow Controller (`underglow_controller.cpp`)
Manages external WS2815/WS2813 LED strips connected to the dashboard to provide ambient lighting and speed-reactive visual feedback.

### Hardware Protocol
*   **Library**: `NeoPixelBus` via I2S/SPI for high-performance, non-blocking LED driving.
*   **Pin**: GPIO 4 (MISO pin on the SD card port).
*   **LED Type**: 120 pixels configured using `NeoWs2813Method` to support the >250us reset pulse required by WS2815s.

### Control Modes
Controlled via `g_vehicle_state.led_mode`:
1.  **OFF (0)**: Sets all pixels to black (0,0,0).
2.  **SOLID (1)**: Static color based on `led_r`, `led_g`, `led_b` and `led_brightness`.
3.  **BREATHING (2)**: Default mode. Smoothly interpolates back and forth between Purple (`#C3B1E1`) and Cyan (`#00CCCC`) using a sine wave function on a continuous angle.
4.  **SPEED REACTIVE (3)**: Shifts the color palette based on real-time speed.
    *   0 km/h: Solid Cyan
    *   40+ km/h: Solid Purple
    *   Intermediate speeds linearly blend between the two.

### Performance Considerations
The `update()` function runs inside the main loop but uses a non-blocking `millis()` check to cap updates to approximately **60 FPS** (16ms delta). This prevents the LED driving logic from starving the LVGL task or CAN bus polling.
