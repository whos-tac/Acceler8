# Original User Request

## 2026-06-23T09:27:27Z

# Teamwork Project Prompt — Draft

> Status: Launched.
> Goal: Teamwork subagent executing task.

Debug and fix the dashboard issue where CAN telemetry accurately displays voltage but fails to display speed.

Working directory: c:\Users\thatw\Documents\Apollo-8\DashBoard
Integrity mode: development

## Requirements

### R1. Analyze Prior Conversations
Read ALL prior conversation transcripts (e.g., `C:\Users\thatw\.gemini\antigravity\brain\5cf1497a-ae31-4b1b-9fcd-ac294acc2dd0\.system_generated\logs\transcript.jsonl` and any others found in the logs directory) to identify the "familiar CAN issue" where speed was not displaying.

### R2. Fix the CAN Speed Bug
Investigate the codebase (e.g., `can_driver.cpp`, `ui_controller.cpp`, telemetry packets) to find the root cause of the speed value not displaying while voltage works fine. 
**Note from user**: "FYI we are connected to the Slave ESC CAN output (as intended)." Implement the fix considering this hardware setup.

## Acceptance Criteria

### Manual Verification
- [ ] A summary of the root cause found in the previous conversation logs is provided.
- [ ] Code modifications have been made to properly calculate, parse, or route the speed data.
- [ ] The dashboard environment compiles successfully without errors (`pio run -e waveshare_dash`).
