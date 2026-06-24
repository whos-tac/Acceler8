# Project: CAN Telemetry Speed Display Fix

## Architecture
- **CAN Bus Integration**: The Dashboard is connected to the Slave ESC CAN output.
- **Data Flow**:
  - Telemetry frames are received from CAN.
  - The CAN driver receives and parses these frames (e.g. VESC CAN telemetry frames, ERPM, voltage).
  - The parsed data is passed to the UI controller or display app which updates LVGL widgets.
  - Current issue: Voltage is accurately displayed, but Speed (derived from ERPM/wheel configuration) fails to display.

## Milestones
| # | Name | Scope | Dependencies | Status | Conversation ID |
|---|------|-------|-------------|--------|-----------------|
| 1 | Log Exploration & Code Audit | Analyze prior conversations for the "familiar CAN issue" context. Inspect `can_driver.cpp`, `ui_controller.cpp`, and other telemetry files. | None | DONE | 912c27de-5e27-4ab1-9ecb-89622b987a5b, b4a5a63e-a7a5-47a9-893b-ac10e6d404a1, e6f644f6-811b-48e2-8d5b-d4fdbd907539 |
| 2 | Bug Fix Implementation | Implement parsing/routing changes to display speed from the Slave ESC CAN output. | M1 | DONE | e60daa28-f6c3-4fbb-9ed7-7571d621fbdb |
| 3 | Build & Verification | Compile via `pio run -e waveshare_dash`. | M2 | DONE | 667d9dfa-873e-4ab4-a66e-9c723960c10e, 3af4de89-b777-43e8-b5e4-a23a6b4b7eab |

## Interface Contracts
- CAN telemetry packets should be decoded correctly based on their CAN ID. Since the connection is to the Slave ESC CAN output, we need to inspect how the receiver parses packages originating from the Slave ESC versus Master ESC.
- Speed is derived from ERPM using the configured wheel size and gear ratio.

## Code Layout
- `src/can/can_driver.cpp` (or similar CAN files)
- `src/ui/ui_controller.cpp` (or similar UI/controller files)
- `src/main.cpp`
