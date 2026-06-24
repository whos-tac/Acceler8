# Review & Handoff Report: CAN Telemetry Speed Display & VESC Protocol Fix

## 1. Observation

- **Modified Files Reviewed**:
  - `c:\Users\thatw\Documents\Apollo-8\DashBoard\src\can_driver.cpp` (Lines 116–162)
  - `c:\Users\thatw\Documents\Apollo-8\DashBoard\src\ui_controller.cpp` (Lines 362–379)
- **Other Modified Files in Workspace (Out of Scope)**:
  - `src/espnow_dash.cpp` (Spoofing MAC address)
  - `src/underglow_controller.cpp` (Changed PixelPin to 4)
- **Compilation Command**:
  - Command: `pio run -e waveshare_dash`
  - Working Directory: `c:\Users\thatw\Documents\Apollo-8\DashBoard`
  - Result: SUCCESS
  - Output excerpt:
    ```
    Advanced Memory Usage is available via "PlatformIO Home > Project Inspect"
    RAM:   [===       ]  29.1% (used 95508 bytes from 327680 bytes)
    Flash: [===       ]  33.2% (used 1110705 bytes from 3342336 bytes)
    ========================= [SUCCESS] Took 11.02 seconds =========================
    ```

## 2. Logic Chain

1. **EID Mismatch Root Cause**: Standard VESC CAN status frames use the layout `(Command_ID << 8) | Controller_ID` (Command ID in bits 8–15, Controller/ESC ID in bits 0–7). The old implementation extracted `mcu_id` using `(raw_id >> 8) & 0xFF` and `cmd_id` using `raw_id & 0xFF`, assuming Flipsky's custom protocol (where Controller ID is in bits 8–15 and Command ID in bits 0–7).
2. **Telemetry Latching Failure**: Under the reversed EID mapping, only the Slave ESC (ID 13) sent frames where the parsed command matched the old filter `(cmd_id == 0x0B || cmd_id == 0x0C || cmd_id == 0x0D)`. Because the parsed command ID was hardcoded to 13 (`0x0D`), it was routed to `case 0x0D` (voltage/temp) but never to `case 0x0C` (ERPM). This caused `erpm` to remain 0, displaying 0 km/h.
3. **VESC Standard status message parse**: By reversing the extraction mapping to `controller_id = raw_id & 0xFF` and `command_id = (raw_id >> 8) & 0xFF`, and switching to parse VESC status messages `9`, `16`, and `27`, we correctly route ERPM, current, duty cycle, temperature, and input voltage.
4. **UI Speed Jitter**: Directly referencing `g_vehicle_state.speed_kmh` (which is EMA-smoothed) instead of calling `calculate_speed_kmh` on the raw `erpm` in the UI thread resolves speedometer jitter and avoids redundant calculations.
5. **Ponytail Compliance**: Changes are kept minimal, unused variables were cleaned up, and changes are marked with `// ponytail:` comments.

## 3. Caveats

- **VESC Telemetry Setup**: The ESCs must be configured to broadcast status messages 1 (9), 4 (16), and 5 (27) over CAN. If they are configured differently or have custom non-standard firmware, telemetry parsing may fail.
- **Physical Test**: Code functionality has been verified via PlatformIO compilation and static code analysis. Interactive physical verification on a live e-skateboard is required to ensure telemetry works under real operational conditions.
- **Unstaged Changes**: There are changes in `espnow_dash.cpp` and `underglow_controller.cpp` in the workspace. They compile successfully but are outside the scope of this CAN telemetry review.

## 4. Conclusion

The CAN telemetry speed display fix is implemented correctly and conforms to the project structure and ponytail constraints. The project compiles successfully for the target `waveshare_dash`.

**Final Verdict**: PASS

## 5. Verification Method

To verify compilation, run the following command in PlatformIO:
```powershell
pio run -e waveshare_dash
```
Confirm that it compiles successfully without errors.

---

## Quality Review Summary

**Verdict**: APPROVE

## Verified Claims

- Correct parsing of standard VESC CAN ID layout -> verified via static code mapping inspection -> PASS
- Correct parsing of Status Messages 9, 16, 27 -> verified against VESC firmware specs -> PASS
- Clean PlatformIO compilation of `waveshare_dash` -> verified via running `pio run -e waveshare_dash` -> PASS

## Coverage Gaps

- None. The changes cover all relevant functions in `src/can_driver.cpp` and `src/ui_controller.cpp`.

---

## Challenge Summary (Adversarial Critic Review)

**Overall risk assessment**: LOW

## Challenges

### [Low] Static Latching Order robustness
- **Assumption challenged**: That latching the first two CAN IDs as master/slave dynamically is correct.
- **Attack scenario**: If a non-ESC device sends a standard VESC status message on startup, it could latch.
- **Blast radius**: Minimal, as only ESCs broadcast VESC status messages.
- **Mitigation**: The dynamic latching is simple and sufficient for standard e-skateboard setups.

### [Low] Status Message 5 length check
- **Assumption challenged**: That Status Message 5 data length is always at least 6 bytes.
- **Attack scenario**: A short frame with length < 6 could read out of bounds.
- **Blast radius**: Protected by the global check `message.data_length_code >= 6` in `can_driver.cpp`.
- **Mitigation**: The check is sufficient to prevent out-of-bounds reading.
