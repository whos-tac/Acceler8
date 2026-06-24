# Handoff Report: CAN Telemetry Speed Display & Voltage Jitter Investigation

## 1. Observation

### Codebase Audits:
1. **File**: `c:\Users\thatw\Documents\Apollo-8\DashBoard\src\can_driver.cpp`
   - **Lines 114–162** (Flipsky Custom Protocol EID Parsing):
     ```cpp
             // Drain the queue completely on every loop to prevent overflow
             while (twai_receive(&message, 0) == ESP_OK) {
                 if (message.extd && message.data_length_code >= 6) {
                     uint32_t raw_id = message.identifier;
                     // Flipsky Layout: Bits 8-15 = ESC ID, Bits 0-7 = Command ID
                     uint8_t mcu_id = (raw_id >> 8) & 0xFF;
                     uint8_t cmd_id = raw_id & 0xFF;
     
                     static int master_id = -1;
                     static int slave_id = -1;
     
                     // Software filter: aggressively ignore 0x00 spam to save CPU cycles
                     if (cmd_id == 0x00) continue;
                     
                     
                     // ============================================================
                     // FLIPSKY CUSTOM TELEMETRY PARSING
                     // ID Layout: Bits 8-15 = ESC ID, Bits 0-7 = Command
                     // ============================================================
                     // Only latch onto IDs if they are sending Flipsky telemetry commands
                     if (cmd_id == 0x0B || cmd_id == 0x0C || cmd_id == 0x0D) {
                         if (master_id == -1) master_id = mcu_id;
                         else if (slave_id == -1 && mcu_id != master_id) slave_id = mcu_id;
                     }
     
                     EscData* target = nullptr;
                     if (mcu_id == master_id) target = &master_esc;
                     else if (mcu_id == slave_id) target = &slave_esc;
     
                     if (target) {
                         target->last_update = now;
     
                         switch (cmd_id) {
                             case 0x0B: // Data 0: Currents (scaled by 1000)
                                 target->motor_current = parseI24(&message.data[0]) / 1000.0f;
                                 target->battery_current = parseI24(&message.data[3]) / 1000.0f;
                                 break;
                             case 0x0C: // Data 1: ERPM & Duty (Flipsky uses 3-bytes for both!)
                                 target->erpm = fabs((float)parseI24(&message.data[0])); 
                                 target->duty = (float)parseI24(&message.data[3]) / 1000.0f; 
                                 break;
                             case 0x0D: // Data 2: Temps & Voltage (scaled by 100)
                                 target->mosfet_temp = parseI16(&message.data[0]) / 100.0f;
                                 target->motor_temp = parseI16(&message.data[2]) / 100.0f;
                                 target->voltage = parseI16(&message.data[4]) / 100.0f;
                                 break;
                         }
                     }
                 }
             }
     ```
   - **Lines 181–183** (ERPM Extraction):
     ```cpp
                 int32_t erpm_m = master_alive ? abs((int32_t)master_esc.erpm) : 0;
                 int32_t erpm_s = slave_alive ? abs((int32_t)slave_esc.erpm) : 0;
                 g_vehicle_state.erpm = (erpm_m > erpm_s) ? erpm_m : erpm_s; // Use highest ERPM for instant response
     ```

2. **File**: `c:\Users\thatw\Documents\Apollo-8\DashBoard\src\ui_controller.cpp`
   - **Line 378** (UI Speed Calculation Bypass):
     ```cpp
     float speed = mock_mode ? mock_speed : calculate_speed_kmh(erpm);
     ```

3. **File**: `c:\Users\thatw\Documents\Apollo-8\DashBoard\include\mechanical_config.h`
   - **Lines 22–28** (Speed Helper):
     ```cpp
     inline float calculate_speed_kmh(int32_t erpm) {
         if (motor_pole_pairs == 0 || gear_ratio == 0.0f) return 0.0f;
         float rpm = (float)erpm / motor_pole_pairs;
         float wheel_rpm = rpm / gear_ratio;
         float circumference_m = (wheel_diameter_mm * PI) / 1000.0f;
         return (wheel_rpm * circumference_m * 60.0f) / 1000.0f;
     }
     ```

### System Logs & Hardware Configuration:
- Physical build has the dashboard CAN interface connected directly to the **Slave ESC CAN output**. The Slave ESC runs standard VESC firmware with VESC CAN ID set to **13 (`0x0D`)**.
- The Master ESC has standard VESC CAN ID (usually **1**) or is controlled via UART, which means its CAN status message broadcasts are either ignored or disabled.
- The `transcript.jsonl` contains files showing telemetry prints like:
  ```
  Flipsky Active: M_ID=163 (40.3V, 0.0A) | S_ID=224 (0.0V, 0.0A) | Sum=0W
  ```
  which is a hardcoded print statement showing `master_esc` variables under the label "M_ID=163" and `slave_esc` under "S_ID=224".

---

## 2. Logic Chain

1. **EID Layout Mismatch**:
   - `can_driver.cpp` is coded to extract:
     - `mcu_id = (raw_id >> 8) & 0xFF` (assumed ESC ID in Flipsky protocol)
     - `cmd_id = raw_id & 0xFF` (assumed Command ID in Flipsky protocol)
   - Standard VESC extended frame layout is the reverse:
     - `controller_id = raw_id & 0xFF` (the ESC's CAN ID, stored in bits 0-7)
     - `command_id = (raw_id >> 8) & 0xFF` (the VESC Command ID, stored in bits 8-15)
   - Therefore, the parsed `cmd_id` extracts the ESC's CAN ID, and the parsed `mcu_id` extracts the VESC Command ID.

2. **Telemetry Latching & Route Overlap**:
   - The latching filter `if (cmd_id == 0x0B || cmd_id == 0x0C || cmd_id == 0x0D)` checks if the parsed `cmd_id` (the ESC's CAN ID) is 11, 12, or 13.
   - The Slave ESC is configured with CAN ID 13 (`0x0D`), which matches the `0x0D` condition.
   - Because the CAN ID is 13, **every single status message** sent by the Slave ESC has its lowest 8 bits equal to `0x0D`.
   - As a result, the parsed `cmd_id` is always `0x0D` for all Slave ESC messages.
   - This routes **all** Slave ESC messages to the switch statement's `case 0x0D` (which is intended for Flipsky Data 2: Temps & Voltage).
   - The code **never** routes to `case 0x0C` (which is intended for ERPM & Duty cycle).

3. **Speed Display Failure & Voltage Jitter**:
   - Since `case 0x0C` is never entered for any message, `target->erpm` is never updated and remains at its default value of 0.
   - Consequently, `g_vehicle_state.erpm` remains 0, and the speedometer always reads `0.0 km/h`.
   - Additionally, since all status messages (Status 1, 4, and 5) from the Slave ESC are routed to `case 0x0D`, they all parse bytes 4-5 of their respective payloads as voltage:
     - **Status Msg 5** (`command_id = 27`): Stores input voltage at bytes 4-5. Voltage is parsed correctly (though scaled incorrectly by `/ 100.0f` instead of `/ 10.0f`).
     - **Status Msg 1** (`command_id = 9`): Stores motor current at bytes 4-5. This overwrites the voltage variable with current data.
     - **Status Msg 4** (`command_id = 16`): Stores input current at bytes 4-5. This also overwrites the voltage variable.
   - This constant overwriting of the voltage variable with current values causes the voltage display to jitter and fluctuate wildly under load.

4. **Cascading Failures**:
   - **Odometer**: Since `g_vehicle_state.erpm` is 0, the distance integration in `odometer.cpp` receives 0.0 speed, keeping the odometer dead.
   - **Range Estimation**: Since distance is 0, efficiency calculations default to the safety fallback of 25 Wh/km.
   - **Underglow LEDs**: Since `g_vehicle_state.speed_kmh` remains 0, speed-reactive LED modes are locked to the slow speed color (Cyan) and never shift.

5. **UI Speed Smoothing Bypass**:
   - In `ui_controller.cpp` line 378, the speed is recalculated via `calculate_speed_kmh(erpm)`. This bypasses the Exponential Moving Average (EMA) smoothing of `g_vehicle_state.speed_kmh` implemented in `can_driver.cpp`. To prevent speedometer jitter once the CAN parser is fixed, this recalculation must be replaced with the EMA-smoothed `g_vehicle_state.speed_kmh`.

---

## 3. Caveats

- We assume the ESCs run standard VESC firmware (or compatible) that broadcasts standard VESC Status Messages (9, 16, 27) using the standard VESC EID layout. If they were running custom Flipsky firmware, the original code would have worked.
- The Master ESC's CAN ID (typically 1) does not match the Flipsky filter (`0x0B/0x0C/0x0D`), so it is ignored. This is acceptable since the dashboard is connected directly to the Slave ESC's CAN output, and the Slave ESC's data is sufficient for speed telemetry.

---

## 4. Conclusion

The root cause of the speed display issue is a mismatch between the dashboard's Flipsky custom CAN layout parser and the standard VESC CAN status messages broadcast by the ESCs. The Slave ESC's CAN ID 13 (`0x0D`) maps all its status messages to the voltage/temperature parser, preventing ERPM from ever being parsed. This also corrupts the voltage reading under load, and breaks the odometer, range estimation, and speed-reactive lighting.

### Recommended Fix Strategy:
1. **Update `src/can_driver.cpp`**:
   Replace the Flipsky-specific bit parsing and latching with standard VESC parsing logic. Update the EID layout parsing to:
   - `controller_id = raw_id & 0xFF`
   - `command_id = (raw_id >> 8) & 0xFF`
   Latch ESC IDs using VESC command IDs 9, 16, and 27, and parse the payloads accordingly:
   - `case 9`: Parse `erpm` (int32 at bytes 0-3), `motor_current` (int16/10.0f at bytes 4-5), and `duty` (int16/1000.0f at bytes 6-7).
   - `case 16`: Parse `mosfet_temp` (int16/10.0f at bytes 0-1), `motor_temp` (int16/10.0f at bytes 2-3), and `battery_current` (int16/10.0f at bytes 4-5).
   - `case 27`: Parse `voltage` (int16/10.0f at bytes 4-5).

2. **Update `src/ui_controller.cpp`**:
   Modify line 378 to use the smoothed speed directly:
   ```cpp
   float speed = mock_mode ? mock_speed : g_vehicle_state.speed_kmh;
   ```
   This avoids redundant calculations and utilizes the EMA filter to smooth out speedometer readings.

---

## 5. Verification Method

### 1. Code Changes & Compilation:
Ensure the patch (proposed by Explorer 2 in `c:\Users\thatw\Documents\Apollo-8\DashBoard\.agents\teamwork_preview_explorer_log_exploration_code_audit_2\proposed_can_driver.patch`) is applied, and verify compilation succeeds:
```powershell
pio run
```

### 2. Interactive Testing:
Run the full-stack simulator using:
```powershell
pio run -e native_full_stack -t exec
```
Inject mock VESC Status messages (IDs `0x090D`, `0x100D`, and `0x1B0D`) and verify that:
- The speedometer displays a stable speed in km/h.
- The battery voltage displays correctly (e.g. 42V and not 4.2V or fluctuating wildly).
- The odometer registers distance in real-time.
- The underglow LEDs transition colors smoothly in response to speed changes.
