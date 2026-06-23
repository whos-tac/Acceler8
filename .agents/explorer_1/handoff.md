# Handoff Report - Explorer Agent

## 1. Observation

### Build Environment Verification
We ran the PlatformIO native build command:
`pio run -e native_full_stack`
The build failed with the following exact compilation errors:

1. In `src/remote/remote_app.cpp:605:33`:
```
src/remote/remote_app.cpp:605:33: error: 'abs' was not declared in this scope
  605 |         int throttle_val = (int)abs(throttle);
      |                                 ^~~
```
2. In `src/settings_screen.cpp:185:26`:
```
src/settings_screen.cpp:185:26: error: '::millis' has not been declared
  185 |         uint32_t now = ::millis();
      |                          ^~~~~~
```

### SDL and App Initialization Setup
- **File**: `src/simulation/sim_main.cpp`
  - Defines 3 separate instances of `SimWindow`: `dash_win`, `remote_win`, `receiver_win`.
  - Initializes each window with a unique title and resolution (`dash_win`: 480x480, `remote_win`: 170x320, `receiver_win`: 400x300).
  - Configures separate LVGL display buffers (e.g. `sim->buf = malloc(w * h / 10 * sizeof(lv_color_t))`) and registers unique drivers.
  - Passes mouse inputs into each display separately via `my_mouse_read`, which references the window's specific `mouse_x`, `mouse_y`, and `mouse_pressed` states.
  - Identifies target windows for mouse events by checking:
    `uint32_t win_id = e.window.windowID;`
    And compares it with `SDL_GetWindowID(win.window)`.

### App Architecture and Telemetry
- **File**: `src/receiver/receiver_app.cpp`
  - If `!defined(ARDUINO)`, it reads from 3 UI sliders to mock telemetry values:
    `float s = slider_speed ? lv_slider_get_value(slider_speed) : 0;`
    `float v = slider_battery ? lv_slider_get_value(slider_battery) : 42.0;`
    `float p = slider_power ? lv_slider_get_value(slider_power) : 0;`
    And sends them to the Dash via:
    `EspnowReceiver::send_mock_telemetry(s, v, p);`
  - Control packets containing the potentiometer-derived throttle percentage are sent from `RemoteApp` to `ReceiverApp` via ESP-NOW.
  - The Receiver translates throttle percentage to UART commands sent to the physical ESC:
    `EscUartDriver::send_throttle(throttle_val, current_gear, current_direction, horn_active, headlight_active, brake_light_active);`
    The UART packet format is structured as Flipsky FTESC protocol V1.6 (STX = 0xAA, CMD = 0x02, 14 bytes data, 2 bytes Modbus CRC-16, ETX = 0xDD).

---

## 2. Logic Chain

1. **Compilation Resolution**:
   - The `abs()` function on native gcc requires `#include <cmath>` or `#include <cstdlib>` and should use `std::abs()` to ensure portability.
   - The settings screen references `::millis()`, which resolves to global namespace `millis()`. In native builds, `millis()` is declared as `extern "C" uint32_t millis()` in `sim_main.cpp`, but `src/settings_screen.cpp` does not declare it for the `#else` block. Adding a proper declaration `extern "C" uint32_t millis();` resolves the compilation error.
2. **Single-Window Integration**:
   - LVGL supports multiple display drivers (`lv_disp_drv_register`) simultaneously.
   - Instead of separate windows, we can initialize a single SDL window of width 1024 and height 600.
   - We allocate three separate SDL_Textures (480x480, 170x320, 400x300) and copy them into the main window renderer at offset panels (`dest_rect`s) using `SDL_RenderCopy`.
   - Mouse events can be translated to relative panel coordinates by checking if the mouse click coordinate falls within the panel's bounding box and subtracting the panel's origin `(x - origin_x, y - origin_y)`.
3. **ESC and Battery Physics Integration**:
   - Instead of feeding fake telemetry from UI sliders, we can capture the serialized bytes from `EscUartDriver::send_throttle` in the native simulation code.
   - By parsing the throttle and gear parameters, we run a virtual ESC/battery model. The model computes ERPM, motor current, and battery voltage sag based on load and SoC.
   - The computed telemetry is passed back to `ReceiverApp` to be broadcast to the Dashboard and Remote, creating a closed-loop system.
4. **Failsafe and Battery Drain**:
   - Signal loss can be simulated by adding a UI button to toggle a boolean flag `sim_signal_lost` in `sim_main.cpp`. When true, it intercepts and drops all virtual ESP-NOW packets sent between the Remote and the Receiver.
   - Battery drain is simulated by integrating `power_w * dt` and subtracting it from a simulated capacity. When the voltage drops below LVC limits, the ESC model cuts power.

---

## 3. Caveats
- This investigation is read-only and did not modify files to verify successful compilation after applying fixes.
- The unit test runner proposal assumes a standalone main file to avoid linking SDL and LVGL libraries, keeping compiling fast and decoupled.

---

## 4. Conclusion
The native build environment has minor compilation issues that can be fixed easily. Combining the virtual displays into a single SDL window with split-screen viewports, integrating a closed-loop physics/battery simulation, mocking signal loss, and creating a lightweight unit test suite can be implemented cleanly with minimum code footprint in /ponytail mode.

---

## 5. Verification Method
- **Compilation Check**: After applying the proposed fixes, verify with `pio run -e native_full_stack`.
- **Test Runner Check**: Verify compilation and execution of unit tests using `g++ src/simulation/unit_tests.cpp src/mechanical_config.cpp -o run_tests` and run `./run_tests`.
