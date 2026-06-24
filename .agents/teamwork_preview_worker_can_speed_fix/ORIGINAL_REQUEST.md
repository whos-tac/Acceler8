## 2026-06-23T11:37:28+02:00
You are a teamwork_preview_worker. Your task is to implement the fix for the dashboard CAN telemetry speed display issue.

Style constraint: Run in /ponytail mode (Lazy senior dev mode: build the minimum that works, avoid unrequested abstractions, mark simplifications with a ponytail: comment).

Specifically:
1. Initialize your progress.md at c:\Users\thatw\Documents\Apollo-8\DashBoard\.agents\teamwork_preview_worker_can_speed_fix\progress.md. Keep it updated with Last visited: [timestamp].
2. Read the handoff reports from:
   - Explorer 2: `c:\Users\thatw\Documents\Apollo-8\DashBoard\.agents\teamwork_preview_explorer_log_exploration_code_audit_2\handoff.md`
   - Explorer 3: `c:\Users\thatw\Documents\Apollo-8\DashBoard\.agents\teamwork_preview_explorer_log_exploration_code_audit_3\handoff.md`
   And the patch file:
   - `c:\Users\thatw\Documents\Apollo-8\DashBoard\.agents\teamwork_preview_explorer_log_exploration_code_audit_2\proposed_can_driver.patch`
3. Modify `src/can_driver.cpp` to parse standard VESC status messages (command_id 9, 16, 27) instead of custom Flipsky protocols (as detailed in the patch).
4. Modify `src/ui_controller.cpp` around line 378 (specifically where speed is calculated/extracted for the UI display) to directly use the EMA-smoothed `g_vehicle_state.speed_kmh` instead of recalculating it using `calculate_speed_kmh(erpm)`. This ensures that the Exponential Moving Average (EMA) smoothing filter in `can_driver.cpp` is used, avoiding speedometer jitter.
5. Compile the code using PlatformIO to verify it compiles successfully:
   `pio run -e waveshare_dash`
   Document the build command and results in your handoff report.
6. Write a completion report and handoff details in `c:\Users\thatw\Documents\Apollo-8\DashBoard\.agents\teamwork_preview_worker_can_speed_fix\handoff.md`.
7. MANDATORY INTEGRITY WARNING: DO NOT CHEAT. All implementations must be genuine. DO NOT hardcode test results, create dummy/facade implementations, or circumvent the intended task. A Forensic Auditor will independently verify your work. Integrity violations WILL be detected and your work WILL be rejected.
8. Use send_message to report back to your parent when done.
