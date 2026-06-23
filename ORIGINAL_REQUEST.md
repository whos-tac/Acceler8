# Original User Request

## Initial Request — 2026-06-22T17:39:01+02:00

Redesign the digital test environment to be fully interactive, simulating the full hardware stack (Remote, Dashboard, ESC, and ESP-NOW/UART communications).

Working directory: `c:\Users\thatw\Documents\Apollo-8\DashBoard`
Integrity mode: development
Style: Run in /ponytail mode (Lazy senior dev mode: build the minimum that works, avoid unrequested abstractions, mark simplifications with a ponytail: comment). Ensure all subagents you spawn also operate in this mode.

## Requirements

### R1. Interactive Simulator UI
Build an interactive LVGL-based split-screen view in the native environment showing both the Remote UI and Dashboard UI simultaneously.

### R2. Simulation Controls & Graphs
Include interactive UI elements (e.g., sliders/buttons) to simulate remote inputs (throttle, switches). Provide real-time visual graphs/logs of the ESC's simulated telemetry (current, RPM, duty cycle).

### R3. Edge Case Mocking
Implement logic to mock connection loss (failsafe triggers) and battery drain over time.

### R4. Architecture Freedom
The architecture of the simulator is up to the team (e.g., single executable vs. multiple communicating executables).

## Acceptance Criteria

### Verification & Testing
- [ ] Automated C++ unit tests are implemented to verify the core logic (throttle mapping, failsafe, battery drain) independently of the UI.
- [ ] All unit tests compile and pass successfully.
- [ ] The full native simulator compiles successfully via PlatformIO (`pio run -e native_full_stack` or new environments created).
- [ ] Running the simulator successfully opens the split-screen UI with functional inputs and visual telemetry graphs.

## Follow-up — 2026-06-23T06:08:00Z

The user reported the following findings and bugs in the simulator implementation:
1. **LVC Chatter / Latch Failure**: LVC triggers, but the recovery check resets it to false in the same update. LVC state machine fails to latch when OCV >= 33.0V.
2. **Euler Integration Instability (ERPM Divergence)**: Large dt values (e.g. dt = 72.0s used in unit tests) cause the ERPM linear filter to diverge to physically impossible speed values (11.52M ERPM).
3. **Layout Conformity Check**: Deviations from PROJECT.md regarding file locations/names (should be `src/simulation/esc_model.h`/`esc_model.cpp` instead of `sim_core.h`/`sim_core.cpp`; should use `test/test_core_logic.cpp` instead of `src/simulation/unit_tests.cpp`; Simulation Control Panel width mismatch).

All unit tests compile and run, but fail these stress/boundary tests. These issues must be addressed.

## Follow-up — 2026-06-23T06:28:46Z

Build a self-contained, browser-based clone of the Apollo-8 / ACCELER8 native simulator that faithfully replicates the three-panel interactive dashboard in HTML + vanilla JS, with no server required.

Working directory: `c:\Users\thatw\Documents\Apollo-8\DashBoard\web_sim`
Integrity mode: development
Style: /ponytail — build the minimum that works; no unrequested abstractions; mark simplifications with a `// ponytail:` comment. Ensure all subagents you spawn also operate in this mode.

## Reference — existing native simulator

The C++ simulation to replicate lives at:
- `src/simulation/sim_main.cpp` — 3-panel SDL layout: Remote (170×320), Dashboard (480×480), Controls/Receiver (320×580)
- `src/simulation/esc_model.h` / `esc_model.cpp` — ESC + battery physics model (SimState struct, ramped throttle, ERPM, motor current, battery voltage/sag, LVC)
- `src/receiver/receiver_app.cpp` — control panel: throttle slider → ESC, comm-loss checkbox, rapid-drain checkbox, recharge button, D-Pad, real-time telemetry chart (current, ERPM, duty cycle, voltage)
- `src/remote/remote_app.cpp` — throttle arc display, button state indicator
- `src/dash_app/` — dashboard display (speed, battery voltage, power)

## Requirements

### R1. Three-panel split UI
The web app displays three bordered panels side-by-side: Remote, Dashboard, and Control Panel — matching the visual layout of the native simulator.

### R2. Interactive ESC Control Panel
The Control Panel must include: a throttle slider (maps to −100% → +100%), a "Comm Loss" toggle, a "Rapid Battery Drain" toggle, a "Recharge" button, and a D-Pad (Up/Down/Left/Right/OK buttons). All controls must be wired to the live simulation state.

### R3. ESC Physics Model
Implement the same physics model as `esc_model.cpp` in JavaScript: ramped throttle (no instant jumps), ERPM calculation, motor current, battery current, voltage sag, capacity drain, and low-voltage cutoff (LVC). Tick at ~20Hz.

### R4. Live Telemetry Chart
The Control Panel shows a real-time scrolling line chart of at least: motor current, ERPM (scaled), and battery voltage — updating at ~20Hz.

### R5. Remote and Dashboard Panels
The Remote panel shows the current throttle arc and button state. The Dashboard panel shows speed (km/h derived from ERPM and wheel config), battery voltage, and power (W).

## Acceptance Criteria

### Functional
- [ ] Opening `index.html` in a browser shows all three panels with no errors in the browser console.
- [ ] Moving the throttle slider causes ERPM and motor current to change in the telemetry chart within 1 second.
- [ ] Enabling "Comm Loss" causes the throttle to coast to 0 (failsafe) and the Remote panel to indicate signal loss.
- [ ] "Rapid Battery Drain" visibly depletes battery voltage in the chart within 10 seconds.
- [ ] "Recharge" resets battery voltage to max (42V).
- [ ] LVC activates (visible indicator) when battery voltage drops below ~32V.
- [ ] The telemetry chart scrolls continuously while the simulation runs.

### Code Quality
- [ ] Single `index.html` file (or minimal files — no build step required).
- [ ] The ESC physics model is implemented as a self-contained JS function/module matching the behavior of `esc_model.cpp`.
