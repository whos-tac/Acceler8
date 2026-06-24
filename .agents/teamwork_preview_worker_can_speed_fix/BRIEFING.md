# BRIEFING — 2026-06-23T11:41:00+02:00

## Mission
Implement the CAN telemetry speed display fix for the dashboard by parsing standard VESC status messages and using the EMA-smoothed vehicle speed in the UI.

## 🔒 My Identity
- Archetype: teamwork_preview_worker
- Roles: implementer, qa, specialist
- Working directory: c:\Users\thatw\Documents\Apollo-8\DashBoard\.agents\teamwork_preview_worker_can_speed_fix
- Original parent: ec3151b9-1f2d-4cbe-8ca7-ab9c263350dc
- Milestone: Implement CAN speed telemetry fix

## 🔒 Key Constraints
- Run in /ponytail mode: build minimum that works, avoid unrequested abstractions, mark simplifications with ponytail comment (`// ponytail: ...`).
- Parse standard VESC status messages (command_id 9, 16, 27) instead of custom Flipsky protocols.
- Use `g_vehicle_state.speed_kmh` in UI display calculation directly, avoiding speedometer jitter.
- Verify with PlatformIO: `pio run -e waveshare_dash`.
- Write completion report in `.agents/teamwork_preview_worker_can_speed_fix/handoff.md`.
- No cheating (no hardcoded test outputs or facade implementations).

## Current Parent
- Conversation ID: ec3151b9-1f2d-4cbe-8ca7-ab9c263350dc
- Updated: 2026-06-23T11:41:00+02:00

## Task Summary
- **What to build**: Modify standard VESC status parsing in `src/can_driver.cpp` (status 9, 16, 27) and update `src/ui_controller.cpp` around line 378 to use `g_vehicle_state.speed_kmh`.
- **Success criteria**: PlatformIO compilation succeeds, and logic correctly implements standard VESC and EMA speed use.
- **Interface contracts**: Standard VESC status packet definitions (command_id 9, 16, 27) and `g_vehicle_state`.
- **Code layout**: `src/can_driver.cpp`, `src/ui_controller.cpp`.

## Change Tracker
- **Files modified**:
  - `src/can_driver.cpp` — Swapped Flipsky custom CAN packet parsing for standard VESC telemetry (packets 9, 16, 27) with auto-latching.
  - `src/ui_controller.cpp` — Directly used the EMA-smoothed `speed_kmh` instead of recalculating via `calculate_speed_kmh(erpm)`.
- **Build status**: PASS (`pio run -e waveshare_dash` compiled successfully in 1m 57s)
- **Pending issues**: None

## Quality Status
- **Build/test result**: PASS. PlatformIO built `waveshare_dash` firmware.bin successfully.
- **Lint status**: Clean (no compiler warnings generated on modified source files).
- **Tests added/modified**: Compilation verification performed.

## Key Decisions Made
- Chose to remove unused local variables (`erpm`, `mock_mode`, `mock_speed`) in `src/ui_controller.cpp` rather than keeping them as dead code, keeping the codebase clean.
- Left simple automatic latching heuristic in place in `src/can_driver.cpp`, marked with a ponytail comment.

## Artifact Index
- `.agents/teamwork_preview_worker_can_speed_fix/handoff.md` — Final completion report and verification details.
