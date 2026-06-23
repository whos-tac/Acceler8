# Project: ACCELER8 Digital Test Environment Redesign

## Architecture
- **Single-Window Split-Screen View**: A single SDL2 window (approx. 1024x600) displaying three panels side-by-side or laid out cleanly:
  - Left panel: Lilygo Remote UI (170x320)
  - Middle panel: Dashboard UI (480x480)
  - Right panel: Simulation Control Panel (320x580)
- **Simulated Hardware Stack**:
  - **Remote**: Generates throttle input from simulated potentiometer and button presses, transmits ControlPacket over ESP-NOW.
  - **Receiver**: Ramps and filters throttle, checks for failsafe, converts to ESC UART commands.
  - **ESC Telemetry Engine**: Physics simulation model converting throttle commands to speed (ERPM), current (Amps), and duty cycle (%).
  - **Battery Model**: Tracks battery voltage (starts at 42.0V), drains based on load current, and supports rapid drain mocking.
  - **ESP-NOW & UART Simulation**: Bridges components via simulated packet channels, allowing communication loss (failsafe testing) on command.
- **Unit Testing**: Command-line based unit test suite to test throttle mapping, failsafe triggers, and battery drain independently of UI.

## Milestones
| # | Name | Scope | Dependencies | Status | Conversation ID |
|---|------|-------|-------------|--------|-----------------|
| 1 | Exploration & Contracts | Verify existing native compiler setup, inspect code detail, define internal test setup | None | DONE | ce014b2d-3032-4c21-b8c9-8034e0fae1cd |
| 2 | Redesigned Simulator | Implement single-window split-screen UI, ESC physics model, battery model, failsafe mock, and controls | M1 | IN_PROGRESS | 9cd6504d-bc20-4075-b464-015a6a7a081a |
| 3 | Unit Tests & Verification | Implement automated C++ unit tests, verify execution, run peer checks and Forensic Audit | M2 | IN_PROGRESS | 9cd6504d-bc20-4075-b464-015a6a7a081a |

## Interface Contracts
### ESC ↔ Receiver UART Simulation
- Receiver compiles UART packet: STX (0xAA) + DLEN (15) + Payload (Command 0x02, Throttle High/Low, switches) + CRC16 + ETX (0xDD)
- Simulation passes this packet to the ESC simulator which unpacks it to update simulated load.

### ESP-NOW Communication Loss Simulation
- Simulated `esp_now_send` intercepts packets. When "Comm Loss" is active, Remote -> Receiver packets are silently dropped.
- After 250ms of no packets, Receiver enters Failsafe state, ramping throttle to 0.

## Code Layout
- `src/simulation/sim_main.cpp`: Entry point for single-window SDL simulation, layout management, mouse coordinate translation, and main tick loop.
- `src/receiver/receiver_app.cpp` & `espnow_receiver.cpp`: Updated to use ESC simulator telemetry instead of mock sliders.
- `src/simulation/esc_model.h` / `esc_model.cpp`: New ESC physical model and battery model.
- `test/test_core_logic.cpp`: New test file containing automated unit tests.
