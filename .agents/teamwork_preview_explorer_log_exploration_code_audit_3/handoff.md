# Handoff Report: CAN Telemetry Speed Display & Secondary Systems Audit

## 1. Observation

### Codebase Audits:
1. **File**: `c:\Users\thatw\Documents\Apollo-8\DashBoard\src\can_driver.cpp`
   - **Lines 114–162** (Flipsky Custom Protocol Parser):
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
   - **Lines 180–210** (Dual ESC Aggregation Logic):
     ```cpp
                 int32_t erpm_m = master_alive ? abs((int32_t)master_esc.erpm) : 0;
                 int32_t erpm_s = slave_alive ? abs((int32_t)slave_esc.erpm) : 0;
                 g_vehicle_state.erpm = (erpm_m > erpm_s) ? erpm_m : erpm_s; // Use highest ERPM for instant response
     
                 if (master_alive) {
                     v += master_esc.voltage;
                     batt_amps += master_esc.battery_current;
                     mot_amps += master_esc.motor_current;
                     max_fet = master_esc.mosfet_temp;
                     max_mot = master_esc.motor_temp;
                     g_vehicle_state.duty_cycle = master_esc.duty;
                     v_count++;
                 }
     
                 if (slave_alive) {
                     v += slave_esc.voltage;
                     batt_amps += slave_esc.battery_current;
                     mot_amps += slave_esc.motor_current;
                     if (slave_esc.mosfet_temp > max_fet) max_fet = slave_esc.mosfet_temp;
                     if (slave_esc.motor_temp > max_mot) max_mot = slave_esc.motor_temp;
                     
                     // If master is dead, duty cycle falls back to slave
                     if (!master_alive) {
                         g_vehicle_state.duty_cycle = slave_esc.duty;
                     }
                     v_count++;
                 }
     ```

2. **File**: `c:\Users\thatw\Documents\Apollo-8\DashBoard\src\ui_controller.cpp`
   - **Lines 363–378** (UI Speed and Voltage Extraction):
     ```cpp
             int32_t erpm = g_vehicle_state.erpm;
             float v      = g_vehicle_state.battery_voltage_v;
             ...
             float speed = mock_mode ? mock_speed : calculate_speed_kmh(erpm);
     ```

3. **File**: `c:\Users\thatw\Documents\Apollo-8\DashBoard\src\odometer.cpp`
   - **Lines 43–45** (Odometer Speed Integration):
     ```cpp
             int32_t current_erpm = g_vehicle_state.erpm;
             DASH_UNLOCK();
             float current_speed = calculate_speed_kmh(fabs((float)current_erpm));
     ```

4. **File**: `c:\Users\thatw\Documents\Apollo-8\DashBoard\src\underglow_controller.cpp`
   - **Lines 105–110** (Speed-Reactive Underglow LEDs):
     ```cpp
         // SPEED REACTIVE
         // Color shifts from Cyan (Slow) to Purple (Fast)
         float speed_kmh = g_vehicle_state.speed_kmh;
         ...
         float speed_factor = speed_kmh / 40.0f; // Scale to 40km/h
     ```

5. **File**: `c:\Users\thatw\Documents\Apollo-8\DashBoard\include\mechanical_config.h`
   - **Lines 22–28** (Speed Calculation Helper):
     ```cpp
     inline float calculate_speed_kmh(int32_t erpm) {
         if (motor_pole_pairs == 0 || gear_ratio == 0.0f) return 0.0f;
         float rpm = (float)erpm / motor_pole_pairs;
         float wheel_rpm = rpm / gear_ratio;
         float circumference_m = (wheel_diameter_mm * PI) / 1000.0f;
         return (wheel_rpm * circumference_m * 60.0f) / 1000.0f;
     }
     ```

6. **File**: `c:\Users\thatw\Documents\Apollo-8\DashBoard\.agents\teamwork_preview_explorer_log_exploration_code_audit_2\proposed_can_driver.patch` (Explorer 2's proposed patch)

---

## 2. Logic Chain

1. **Reversed EID Bit Mapping**: 
   The original parser in `can_driver.cpp` extracts `mcu_id = (raw_id >> 8) & 0xFF` and `cmd_id = raw_id & 0xFF` assuming a Flipsky custom CAN layout (where Bits 8-15 = ESC ID and Bits 0-7 = Command ID).
   However, standard VESC firmware (which runs on the project ESCs) reverses these bit positions:
   - Bits 8-15: **Command ID** (e.g. 9 for `CAN_PACKET_STATUS_MSG`, 16 for `CAN_PACKET_STATUS_MSG_4`, 27 for `CAN_PACKET_STATUS_MSG_5`)
   - Bits 0-7: **Controller ID** (ESC CAN ID, e.g. 1 for Master, 13 for Slave)
   
   Consequently:
   - `mcu_id` in `can_driver.cpp` extracts the **VESC Command ID**.
   - `cmd_id` in `can_driver.cpp` extracts the **ESC CAN ID**.

2. **Telemetry Latching Mismatch**:
   - The latching code `if (cmd_id == 0x0B || cmd_id == 0x0C || cmd_id == 0x0D)` only executes if the parsed `cmd_id` (which is the ESC CAN ID) matches 11 (`0x0B`), 12 (`0x0C`), or 13 (`0x0D`).
   - The Master ESC's CAN ID is standard (usually 1), so all its messages are ignored by this check.
   - The Slave ESC's CAN ID is 13 (`0x0D`), which matches the `0x0D` condition. Thus, only the Slave ESC's messages are processed by the latching logic.
   - When the Slave ESC (ID 13) sends a Status Msg 5 (Command ID 27):
     - `mcu_id` is parsed as 27.
     - `cmd_id` is parsed as 13 (`0x0D`).
     - Latching logic sets `master_id = mcu_id = 27`.
   - When the Slave ESC (ID 13) sends a Status Msg 1 (Command ID 9):
     - `mcu_id` is parsed as 9.
     - `cmd_id` is parsed as 13 (`0x0D`).
     - Latching logic sets `slave_id = mcu_id = 9`.

3. **Voltage Logic and Jitter Corruption**:
   - Because `master_id` is latched to 27, any message with `mcu_id = 27` (Slave Status Msg 5) targets `master_esc`. The switch on `cmd_id` (13 = `0x0D`) enters `case 0x0D` and parses `target->voltage = parseI16(&message.data[4]) / 100.0f;`. Because VESC Status Msg 5 stores input voltage at bytes 4-5, it successfully reads the voltage (though scaled incorrectly by `/ 100.0f` instead of `/ 10.0f`).
   - Because `slave_id` is latched to 9, any message with `mcu_id = 9` (Slave Status Msg 1) targets `slave_esc`. It goes into `switch (cmd_id = 0x0D)` -> `case 0x0D`, and parses `target->voltage = parseI16(&message.data[4]) / 100.0f;`. However, VESC Status Msg 1 has motor current at bytes 4-5, not voltage. This results in the Slave ESC's voltage being overwritten with motor current data. Under load, the average voltage calculation `(master_esc.voltage + slave_esc.voltage) / 2` is corrupted by the motor current value, causing voltage display jitter/errors.

4. **Speed Display Failure**:
   - For all incoming messages from the Slave ESC, the parsed `cmd_id` is 13 (`0x0D`).
   - The switch statement `switch (cmd_id)` always routes to `case 0x0D`. It NEVER enters `case 0x0C` (the speed/ERPM parser).
   - As a result, both `master_esc.erpm` and `slave_esc.erpm` remain at their initialized value of 0.
   - `g_vehicle_state.erpm` remains 0, making the speedometer always read 0.0 km/h.

5. **Secondary Cascading Failures**:
   - **Odometer**: Since `g_vehicle_state.erpm` is 0, `current_speed` integrated in `odometer.cpp` is 0.0 km/h, meaning the odometer remains completely dead.
   - **Range Estimation**: Since distance remains 0, the real-time efficiency calculation is invalid, forcing the range estimator to always fall back to the safety default of 25 Wh/km.
   - **Underglow LEDs**: Since `g_vehicle_state.speed_kmh` remains 0, the speed-reactive lighting mode is stuck on the "Slow" (Cyan) color and never shifts to "Fast" (Purple).
   - **Redundant UI Bypass**: In `ui_controller.cpp` line 378, speed is recalculated via `calculate_speed_kmh(erpm)`. This bypasses the Exponential Moving Average (EMA) smoothing of `g_vehicle_state.speed_kmh` implemented in `can_driver.cpp`, meaning that even after correcting the CAN parsing, the UI speed reading would exhibit jitter unless this UI bypass is replaced with `g_vehicle_state.speed_kmh`.

---

## 3. Caveats

- We assume the ESCs are running standard VESC-compatible firmware which broadcasts standard VESC Status Messages (9, 16, 27) rather than custom Flipsky protocol messages. If the ESCs were running custom Flipsky firmware, the original code would function correctly.
- If the Master ESC is configured to standard VESC default CAN ID (1) but has its status messages disabled or is not connected to the bus, the automatic ID latching strategy still functions using the Slave ESC's messages alone.

---

## 4. Conclusion

The root cause of the speed display issue is a mismatch between the dashboard's Flipsky custom CAN layout parser and the standard VESC CAN status messages broadcast by the ESCs. The Slave ESC's CAN ID 13 (`0x0D`) accidentally bypasses the command filter but routes all status messages to the temperature/voltage parser, leaving ERPM/speed un-updated. This also breaks the odometer, range estimation, and speed-reactive underglow lighting, and corrupts the average voltage reading under load.

### Fix Strategy:
1. **Update `src/can_driver.cpp`**: 
   Replace the Flipsky-specific bit parsing and latching with standard VESC parsing logic. Use VESC Command IDs 9, 16, and 27 to latch onto ESC CAN IDs, and parse the data fields (big-endian) with the correct scaling factors.
   Applying Explorer 2's patch (`proposed_can_driver.patch`) achieves this:
   - Sets `controller_id = raw_id & 0xFF`
   - Sets `command_id = (raw_id >> 8) & 0xFF`
   - Latches `master_id` and `slave_id` using `command_id` values 9, 16, or 27.
   - Parses `erpm` and `duty` from `command_id == 9`.
   - Parses `mosfet_temp`, `motor_temp`, and `battery_current` from `command_id == 16`.
   - Parses `voltage` (divided by 10.0f) from `command_id == 27`.
2. **Update `src/ui_controller.cpp`**:
   Modify line 378 to use the EMA-smoothed `g_vehicle_state.speed_kmh` directly:
   `float speed = mock_mode ? mock_speed : g_vehicle_state.speed_kmh;`
   This eliminates redundant calculation overhead and ensures the UI utilizes the EMA smoothing filter designed to prevent speedometer jitter.

---

## 5. Verification Method

### 1. Verification of Code Changes:
Inspect `src/can_driver.cpp` and `src/ui_controller.cpp` to ensure the patch is applied.
Confirm compilation succeeds by running:
```bash
pio run
```

### 2. Physical/Interactive Testing:
- Launch the simulator or flash the dashboard:
  ```bash
  pio run -e native_full_stack -t exec
  ```
- Send simulated VESC CAN messages corresponding to IDs `0x090D`, `0x100D`, and `0x1B0D` (representing VESC Status 1, 4, and 5 from Slave ESC ID 13).
- Verify that:
  - The speedometer updates smoothly in km/h.
  - The voltage reads accurately (e.g. 42V and not 4.2V or fluctuating wildly under load).
  - The odometer records distance in real-time.
  - The underglow LEDs react to speed, shifting colors from Cyan to Purple.
