# BRIEFING — 2026-06-23T11:28:30+02:00

## Mission
Investigate dashboard speed display issue with CAN telemetry and formulate a fix strategy.

## 🔒 My Identity
- Archetype: Teamwork explorer (Log and Code Explorer 1)
- Roles: Log exploration, Code audit
- Working directory: c:\Users\thatw\Documents\Apollo-8\DashBoard\.agents\teamwork_preview_explorer_log_exploration_code_audit_1
- Original parent: ec3151b9-1f2d-4cbe-8ca7-ab9c263350dc
- Milestone: Investigation and Fix Strategy

## 🔒 Key Constraints
- Read-only investigation — do NOT implement
- Operating in CODE_ONLY network mode

## Current Parent
- Conversation ID: ec3151b9-1f2d-4cbe-8ca7-ab9c263350dc
- Updated: not yet

## Investigation State
- **Explored paths**: `C:\Users\thatw\.gemini\antigravity\brain\5cf1497a-ae31-4b1b-9fcd-ac294acc2dd0\.system_generated\logs\`, `src/can_driver.cpp`, `src/ui_controller.cpp`, `include/mechanical_config.h`
- **Key findings**: Identified that the dashboard uses Flipsky EID bit layout (`mcu_id = (raw_id >> 8) & 0xFF`, `cmd_id = raw_id & 0xFF`) while the ESCs broadcast standard VESC status messages (`controller_id = raw_id & 0xFF`, `command_id = (raw_id >> 8) & 0xFF`). Since the Slave ESC has CAN ID 13 (`0x0D`), all its messages are interpreted as command `0x0D` (Flipsky Data 2), bypassing ERPM parsing and constantly corrupting voltage with current.
- **Unexplored areas**: None, investigation complete.

## Key Decisions Made
- Confirmed Explorer 2's diagnosis and recommended applying the standard VESC EID parsing patch in `can_driver.cpp`.
- Proposed additional improvement in `ui_controller.cpp` to use the EMA-smoothed `speed_kmh` instead of bypassing the filter.

## Artifact Index
- c:\Users\thatw\Documents\Apollo-8\DashBoard\.agents\teamwork_preview_explorer_log_exploration_code_audit_1\progress.md — Progress log / heartbeat
- c:\Users\thatw\Documents\Apollo-8\DashBoard\.agents\teamwork_preview_explorer_log_exploration_code_audit_1\handoff.md — Final handoff report containing findings and recommended fix strategy
