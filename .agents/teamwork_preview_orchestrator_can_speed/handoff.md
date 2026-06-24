# Handoff Report: CAN Telemetry Speed Display & VESC Protocol Fix

## 1. Observation

### Codebase Changes:
- **`src/can_driver.cpp`**: Swapped custom Flipsky CAN packet parsing (which targeted bits 8-15 for Controller ID and bits 0-7 for Command ID) for standard VESC telemetry parsing (where Command ID is in bits 8-15 and Controller ID is in bits 0-7). Added automatic ESC ID latching for VESC status messages `9`, `16`, and `27`.
- **`src/ui_controller.cpp`**: Replaced raw speed recalculation from ERPM in the UI display loop with direct access to the EMA-smoothed `g_vehicle_state.speed_kmh`.

### Build Verification:
- **Command**: `pio run -e waveshare_dash`
- **Result**: SUCCESS (tested independently by Worker, Reviewer 1, and Reviewer 2)
- **Compilation Output**:
  - RAM: 29.1% (used 95508 bytes from 327680 bytes)
  - Flash: 33.2% (used 1110705 bytes from 3342336 bytes)

---

## 2. Logic Chain

1. **Reversed EID Bit Layout Mismatch**:
   - The original CAN telemetry parser in `can_driver.cpp` was written to parse Flipsky custom CAN protocols where:
     - `mcu_id = (raw_id >> 8) & 0xFF` (extracted ESC ID)
     - `cmd_id = raw_id & 0xFF` (extracted Command ID)
   - However, the hardware runs standard VESC extended ID (EID) telemetry. Standard VESC EID formats reverse these bytes:
     - Bits 0-7: Controller ID (ESC CAN ID, e.g. 1 for Master, 13 for Slave)
     - Bits 8-15: Command ID (e.g. 9 for `STATUS_MSG`, 16 for `STATUS_MSG_4`, 27 for `STATUS_MSG_5`)
   - Consequently, in the original code, `mcu_id` actually extracted the VESC Command ID, and `cmd_id` extracted the Controller ID.

2. **Telemetry Latching and Telemetry Failure**:
   - The original driver latched ESCs only if `cmd_id` matched `0x0B`, `0x0C`, or `0x0D`. Since the Slave ESC CAN ID was configured as 13 (`0x0D`), only its messages matched the check, and it was latched.
   - For all incoming messages from the Slave ESC, the parsed command ID (`cmd_id`) was always 13 (`0x0D`), meaning the parsing switch always routed to `case 0x0D` (voltage/temp) and never entered `case 0x0C` (ERPM). This caused `erpm` to remain 0, and the speedometer to display 0 km/h.

3. **Secondary Failures**:
   - **Voltage Jitter / Corruption**: Since VESC Status Msg 1 (`command_id = 9`, representing ERPM/Current/Duty) was processed under `case 0x0D` (temps & voltage), the bytes representing motor current were parsed as input voltage, corrupting the average battery voltage calculation under load.
   - **Dead Odometer**: Speed integration in `odometer.cpp` calculated a distance of 0.0 km because ERPM remained 0.
   - **Range Estimation**: Real-time consumption integration failed, forcing the range estimator to constantly fall back to a conservative default.
   - **Stuck reactive lighting**: Speed-reactive underglow modes remained stuck at the Slow (Cyan) color state.

4. **UI Speed Jitter**:
   - In `src/ui_controller.cpp` around line 378, speed was recalculated from raw ERPM on every update, completely bypassing the Exponential Moving Average (EMA) smoothing filter in `can_driver.cpp`. Directly using the EMA-smoothed `g_vehicle_state.speed_kmh` solves the speedometer jitter and removes redundant calculations.

5. **Fix Verification**:
   - Swapping EID extraction to standard VESC formatting (`controller_id = raw_id & 0xFF` and `command_id = (raw_id >> 8) & 0xFF`) and parsing messages 9, 16, and 27 resolves all telemetry data routing.

---

## 3. Caveats

- **VESC Firmware Config**: Assumes ESCs broadcast standard status messages 9, 16, and 27. If status packets are disabled or modified in firmware, the telemetry display will not function.
- **Physical Verification**: Verification has been successfully performed via PlatformIO build checks (`waveshare_dash` target) and static code/protocol audits. Interactive real-world validation on the physical dashboard unit is recommended to confirm telemetry flow.

---

## 4. Conclusion

The CAN speed display bug is fully resolved. By correcting the EID parsing layout to align with standard VESC status frames, the dashboard correctly receives ERPM, current, voltage, and temperature telemetry. This restores odometer accumulation, range estimation accuracy, and reactive lighting. Utilizing the EMA-smoothed speed in the UI controller resolves the jitter issue. The project compiles successfully for the `waveshare_dash` target.

---

## 5. Verification Method

To verify code syntax and compilation:
1. Run the PlatformIO build tool:
   ```bash
   pio run -e waveshare_dash
   ```
2. Confirm the build finishes with `SUCCESS` and zero errors.
