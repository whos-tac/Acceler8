# BRIEFING — 2026-06-23T09:29:00Z

## Mission
Investigate the dashboard issue where CAN telemetry accurately displays voltage but fails to display speed, and formulate a detailed fix strategy.

## 🔒 My Identity
- Archetype: Log and Code Explorer 3
- Roles: Log and Code Explorer
- Working directory: c:\Users\thatw\Documents\Apollo-8\DashBoard\.agents\teamwork_preview_explorer_log_exploration_code_audit_3
- Original parent: ec3151b9-1f2d-4cbe-8ca7-ab9c263350dc
- Milestone: VESC CAN Speed Display Investigation

## 🔒 Key Constraints
- Read-only investigation — do NOT implement
- CODE_ONLY network mode: no external web access, no curl/wget/lynx to external URLs.

## Current Parent
- Conversation ID: ec3151b9-1f2d-4cbe-8ca7-ab9c263350dc
- Updated: 2026-06-23T09:28:30Z

## Investigation State
- **Explored paths**: `5cf1497a-ae31-4b1b-9fcd-ac294acc2dd0` log folder, `src/can_driver.cpp`, `src/ui_controller.cpp`, `src/odometer.cpp`, `src/underglow_controller.cpp`, `include/mechanical_config.h`
- **Key findings**: Identified that bits are reversed between Flipsky EID and standard VESC status messages, causing Slave VESC (CAN ID 13) status messages to be interpreted as voltage/temp updates only, bypassing speed/ERPM. Documented cascading failures in odometer, range estimation, underglow LEDs, and UI EMA filter bypass.
- **Unexplored areas**: None, investigation complete.

## Key Decisions Made
- Formulated fix strategy to apply standard VESC bit parsing patch to `can_driver.cpp` and update `ui_controller.cpp` to use the EMA-smoothed speed.

## Artifact Index
- c:\Users\thatw\Documents\Apollo-8\DashBoard\.agents\teamwork_preview_explorer_log_exploration_code_audit_3\handoff.md — Final investigation findings and recommended fix strategy
