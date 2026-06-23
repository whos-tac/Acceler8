# Project: ACCELER8 Browser-Based Simulator Clone

## Architecture
- **Single-Window split layout**: A single HTML page (`index.html`) styling three side-by-side bordered divs mimicking the native SDL window layout:
  - Left Panel: Remote UI (throttle arc, button status indicator, signal state).
  - Middle Panel: Dashboard UI (telemetry: speed in km/h, voltage, power).
  - Right Panel: Control Panel (throttle slider, D-pad, checkboxes for Comm Loss / Rapid Drain, Recharge button, real-time scrolling canvas chart).
- **Physics Engine**: JavaScript port of `esc_model.cpp` running at 20Hz (~50ms ticks). Replicates:
  - Ramped throttle input filtering.
  - Odometer & Speed calculations.
  - Telemetry: Voltage sag under load, motor current, battery capacity depletion.
  - LVC latching logic: LVC triggers below 32V and latches until Recharge, preventing LVC chatter when voltage recovers under no-load.
- **E2E / Integration Tests**: Self-contained Node.js unit tests (`test_physics.js`) asserting core physics model logic, failsafes, and LVC latching.

## Milestones
| # | Name | Scope | Dependencies | Status | Conversation ID |
|---|---|---|---|---|---|
| 1 | Exploration & Physics Extraction | Inspect native C++ code to extract math formulas, parameters, UI layouts, and document them | None | DONE | 73e44b58-0eb9-44de-abf3-fe474fe90540 |
| 2 | Core Physics & Automated Tests | Implement `esc_model.js` and write `test_physics.js` unit tests to verify logic | M1 | DONE | 61159e25-7df6-402f-9434-fc7cb2681f2b |
| 3 | UI Layout & Canvas Renderers | Build HTML/CSS layout and custom Canvas drawing functions (throttle arc, scrolling chart) | M2 | IN_PROGRESS | |
| 4 | Integration & E2E Verification | Wire simulation loop, inputs, controls, verify against acceptance criteria, run Forensic Audit | M3 | PLANNED | |

## Interface Contracts
### Controller ↔ ESC State Packet (Simulated)
- The application updates the state at 20Hz by calling `esc.tick(dt, throttleInput, commLossActive, rapidDrainActive)`.
- If `commLossActive` is true, the input throttle to the ESC is zeroed (failsafe) after 250ms signal loss timeout, simulating dropping remote packets.
- When `rapidDrainActive` is true, battery capacity depletion is multiplied by 100x.
- When `recharge` is clicked, the battery state is reset: cell voltage back to full (4.2V/cell, 42V total) and LVC latch is reset.

## Code Layout
- `web_sim/index.html`: Main HTML file detailing layout, UI panels, container elements, and D-pad.
- `web_sim/style.css`: Styles for the split UI panels (Remote, Dashboard, Control Panel).
- `web_sim/esc_model.js`: ESC physical model, battery drain/sag, LVC latch state machine.
- `web_sim/ui_renderers.js`: Custom Canvas renderers for the scrolling chart and throttle arc.
- `web_sim/app.js`: State manager, event listeners, and main 20Hz loop.
- `web_sim/test_physics.js`: Unit tests for the ESC physics model.
