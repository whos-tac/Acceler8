# BRIEFING — 2026-06-22T15:45:12Z

## Mission
Verify the native_full_stack compilation environment and explore the codebase to propose designs for SDL single window split-screen, telemetry ESC engine, communication loss, and C++ unit tests in /ponytail mode.

## 🔒 My Identity
- Archetype: teamwork_preview_explorer
- Roles: explorer, investigator
- Working directory: c:\Users\thatw\Documents\Apollo-8\DashBoard\.agents\explorer_1
- Original parent: a461c114-de12-4448-9775-4cf697f95bde
- Milestone: Explore codebase & verify native compile

## 🔒 Key Constraints
- Read-only investigation — do NOT implement
- Operate in /ponytail mode: propose the minimum that works, avoid unrequested abstractions, mark simplifications with `// ponytail:`. Do NOT modify any code.

## Current Parent
- Conversation ID: a461c114-de12-4448-9775-4cf697f95bde
- Updated: 2026-06-22T17:50:00+02:00

## Investigation State
- **Explored paths**:
  - `platformio.ini` (native_full_stack environment, build flags, library dependencies)
  - `src/simulation/sim_main.cpp` (multiple SDL windows setup, display flush, mouse input read, main loop)
  - `src/dash_app.cpp`, `include/dash_app.h` (dashboard app initialization and updates)
  - `src/remote/remote_app.cpp`, `include/remote_app.h` (remote UI, potentiometer mapping, control packet sending)
  - `src/receiver/receiver_app.cpp`, `include/receiver_app.h` (receiver control loop, failsafe, ramping, mock telemetry)
  - `src/receiver/esc_uart_driver.cpp`, `include/esc_uart_driver.h` (Flipsky FTESC UART protocol serialization, CRC-16)
  - `src/can_driver.cpp`, `include/can_driver.h` (global VehicleState and CAN decoding structure)
- **Key findings**:
  - Compilation fails due to `abs` missing std/cmath inclusion in `remote_app.cpp:605` and `::millis()` missing global declaration in `settings_screen.cpp:185`.
  - The simulation currently displays three separate SDL windows, matching window IDs to route mouse events.
  - The Receiver app uses 3 on-screen sliders (`slider_speed`, `slider_battery`, `slider_power`) to mock telemetry.
- **Unexplored areas**:
  - Unit tests or tests folders in the workspace (`remote_test`, `uart_test`).

## Key Decisions Made
- Propose side-by-side single-window SDL layout (approx 1024x600/1080x600) with destination rect coordinate translation.
- Intercept UART packets inside the simulated `EscUartDriver::send_throttle` to drive a memory-resident ESC/battery physical simulation.
- Propose a UI button to trigger communication loss by blocking ESP-NOW routing in `sim_main.cpp`.
- Design a standalone C++ console unit test runner under a lightweight `native_tests` PlatformIO environment.

## Artifact Index
- c:\Users\thatw\Documents\Apollo-8\DashBoard\.agents\explorer_1\BRIEFING.md — Persistent memory index
- c:\Users\thatw\Documents\Apollo-8\DashBoard\.agents\explorer_1\progress.md — Liveness and step tracking
- c:\Users\thatw\Documents\Apollo-8\DashBoard\.agents\explorer_1\handoff.md — Handoff report
