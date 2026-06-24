# BRIEFING — 2026-06-23T11:45:00+02:00

## Mission
Investigate the dashboard issue where CAN telemetry displays voltage but fails to display speed.

## 🔒 My Identity
- Archetype: Log and Code Explorer 2
- Roles: Read-only investigator
- Working directory: c:\Users\thatw\Documents\Apollo-8\DashBoard\.agents\teamwork_preview_explorer_log_exploration_code_audit_2
- Original parent: b4a5a63e-a7a5-47a9-893b-ac10e6d404a1
- Milestone: Log exploration and code audit for speed display issue

## 🔒 Key Constraints
- Read-only investigation — do NOT implement
- Analyze problems, synthesize findings, produce structured reports
- Follow the 5-component handoff report structure

## Current Parent
- Conversation ID: b4a5a63e-a7a5-47a9-893b-ac10e6d404a1
- Updated: 2026-06-23T11:45:00+02:00

## Investigation State
- **Explored paths**:
  - `C:\Users\thatw\.gemini\antigravity\brain\5cf1497a-ae31-4b1b-9fcd-ac294acc2dd0\.system_generated\logs\transcript.jsonl` (searched for CAN telemetry / speed / ESC settings)
  - `c:\Users\thatw\Documents\Apollo-8\DashBoard\src\can_driver.cpp` (analyzed Flipsky custom CAN protocol parsing and aggregation logic)
  - `c:\Users\thatw\Documents\Apollo-8\DashBoard\src\ui_controller.cpp` (analyzed UI update and speed display logic)
  - `c:\Users\thatw\Documents\Apollo-8\DashBoard\include\mechanical_config.h` (analyzed `calculate_speed_kmh` function)
  - `c:\Users\thatw\Documents\Apollo-8\DashBoard\Resources\FLIPSKY FTESC CAN Protocol V1.4 English.pdf` (extracted and analyzed protocol specification)
- **Key findings**:
  - The dashboard code in `can_driver.cpp` is written assuming the Flipsky custom CAN protocol format, where MCU ID is in bits 8-15 and Command ID is in bits 0-7.
  - In a standard VESC CAN network, the EID structure has Command ID in bits 8-15 and Controller ID (ESC ID) in bits 0-7.
  - This reversed mapping causes the Command ID to be treated as `mcu_id` and the Controller ID to be treated as `cmd_id`.
  - When the Slave ESC (configured with CAN ID 13 / `0x0D`) broadcasts standard VESC status messages:
    - Status Msg 5 (Command ID 27, Controller ID 13) has `mcu_id = 27` and `cmd_id = 13 = 0x0D`. It latches `slave_id = 27` and enters `case 0x0D` (voltage), successfully reading voltage from bytes 4-5 (which aligns with VESC Status 5).
    - Status Msg 1 (Command ID 9, Controller ID 13) has `mcu_id = 9` and `cmd_id = 13 = 0x0D`. It enters `case 0x0D` (parsed as voltage, resulting in garbage) instead of `case 0x0C` (ERPM).
    - As a result, the Slave ESC's ERPM is never parsed. Since the Master ESC (CAN ID 1) is ignored or disabled, speed remains 0.
- **Unexplored areas**: None. The root cause is fully identified.

## Key Decisions Made
- Identified standard VESC CAN extended ID structure mismatch as the root cause of the telemetry issue.
- Formulated fix strategy to update `can_driver.cpp` to parse standard VESC CAN status messages (Command ID in bits 8-15, Controller ID in bits 0-7).

## Artifact Index
- c:\Users\thatw\Documents\Apollo-8\DashBoard\.agents\teamwork_preview_explorer_log_exploration_code_audit_2\handoff.md — Analysis and fix strategy report
