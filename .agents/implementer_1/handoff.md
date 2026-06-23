# Handoff Report — Peer Review Bug Fixes & Layout Compliance

## 1. Observation
- Received peer review findings requesting fixes for the redesigned digital test environment:
  - ESP-NOW Sender Deduction Bug in `src/simulation/sim_main.cpp`.
  - LVC chatter bug in `src/simulation/sim_core.cpp`.
  - Euler Integration Instability/ERPM Divergence in `src/simulation/sim_core.cpp`.
  - Brake Throttle Scaling in `src/simulation/sim_core.cpp`.
  - NaN checks on throttle inputs in both the receiver path and physics loop.
- Received subsequent instruction to:
  - Rename `src/simulation/sim_core.h`/`.cpp` to `src/simulation/esc_model.h`/`.cpp` and update include references. Delete old files.
  - Move unit tests from `src/simulation/unit_tests.cpp` to `test/test_core_logic.cpp`. Delete old unit test file.
  - Change Controls panel width from `340` to `320` inside `sim_main.cpp` and `receiver_app.cpp`.
  - Latched LVC to only clear when capacity is recharged to `state.capacity_ah >= 9.9f`.
  - Use the analytical integration solution for ERPM:
    `state.erpm = target_erpm - (target_erpm - state.erpm) * std::exp(-2.0f * dt);`
- In `src/simulation/sim_main.cpp`, we observed `esp_now_send` originally routed all packets targeting `receiver_mac` as coming from `remote_mac`, which ignored `EscConfigPacket` from `dash_mac` (because `EscConfigPacket` has a size of 4 bytes, while the receiver checks for `ControlPacket` size of 8 bytes).
- Ran unit tests using `pio run -e native_tests` and `.pio\build\native_tests\program.exe`, which completed successfully with all test suites passing.
- Ran full stack build using `pio run -e native_full_stack`, which completed successfully with no compilation errors.

## 2. Logic Chain
- **ESP-NOW sender MAC**: By inspecting `espnow_receiver.cpp`, we saw that the receiver differentiates packets using the sender's MAC. Specifically, `ControlPacket` (size = 8) comes from `remote_mac`, and `EscConfigPacket` (size = 4) comes from `dash_mac`. Deducing the sender MAC inside `esp_now_send` based on length ensures correct routing.
- **Latched LVC**: In the original battery model, voltage sag dropped under load, causing LVC to trigger. Once LVC triggered, current went to 0, which eliminated voltage sag and caused the voltage to recover immediately above 33.0V, resetting LVC and starting a chatter/stuttering cycle. Latching LVC to only clear when `state.capacity_ah >= 9.9f` (recharge) prevents this cycle.
- **ERPM Analytical Integration**: The ERPM first-order filter differential equation is `d(erpm)/dt = 2.0 * (target - erpm)`. Using Euler integration `erpm += (target - erpm) * dt * 2.0` diverges when `dt > 0.5`. The analytical integration `erpm = target - (target - erpm) * exp(-2.0 * dt)` is exact and stable for any `dt` value, including the `72.0s` used in unit tests.
- **Brake Scaling**: In the receiver application's forward path, brake throttle (negative throttle) is scaled by `(MAX_BRAKE_CURRENT_A / MAX_DRIVE_CURRENT_A)` which corresponds to `(50.0f / 50.0f)`. Applying this scaling in `calculate_ramped_throttle` ensures consistency with the receiver's scaling behavior.
- **NaN checking**: Adding checks for `std::isnan(throttle)` in both the simulator core (`calculate_ramped_throttle` and `map_pot_to_throttle`) and the receiver application native loop sanitizes bad inputs and avoids undefined physical behavior.
- **Layout Conformance**: Renaming `sim_core` to `esc_model` and relocating tests to `test/test_core_logic.cpp` complies with the project's layout specifications, ensuring clean directory structure.
- **Controls Width and D-pad centering**: Reducing the virtual display width to `320` in `sim_main.cpp` necessitated scaling elements to `300` in `receiver_app.cpp` (recharge button, chart) and moving D-pad buttons center-x from 170 to 160 to prevent overlap and maintain symmetry.

## 3. Caveats
- The native UI runs in software rendering mode. High GPU usage or lagging frames could occur if the computer is overloaded, but the analytical filter guarantees the simulation variables remain mathematically stable regardless of frame delays.

## 4. Conclusion
- All five peer review issues plus the subsequent layout and analytical filter requirements have been implemented fully and cleanly. The system conforms to the requested layout, the simulator width is adjusted to 320, and all unit tests build and pass successfully.

## 5. Verification Method
- Build unit tests:
  `pio run -e native_tests`
- Run unit tests:
  `.pio\build\native_tests\program.exe`
- Build native full stack:
  `pio run -e native_full_stack`
