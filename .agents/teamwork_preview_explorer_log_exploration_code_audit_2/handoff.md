# Handoff Report: CAN Telemetry Speed Display Issue Audit

## 1. Observation

### Codebase Audited:
- **File**: `c:\Users\thatw\Documents\Apollo-8\DashBoard\src\can_driver.cpp`
- **Lines 114–162**:
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

### PDF Documentation Audited:
- **File**: `c:\Users\thatw\Documents\Apollo-8\DashBoard\Resources\FLIPSKY FTESC CAN Protocol V1.4 English.pdf`
- **Page 4 (EID Layout)**:
  `EID: 29 bits. Bit0~bit7 are used for CAN ID command, bit8~bit15 are used for ESC ID.`
  - bits 8-15: MCU ID (ESC ID)
  - bits 0-7: CAN COMMAND ID

- **Page 11 (Real-time data 1 and 2)**:
  - `CAN_ESC_REALTIME_DATA_1_CRC16 (0CH)`
    - D0 D1 D2: Motor erpm (3 bytes)
    - D3 D4 D5: Duty * 1000 (3 bytes)
  - `CAN_ESC_REALTIME_DATA_2_CRC16 (0DH)`
    - D0 D1: MOSFET Temp * 100 (2 bytes)
    - D2 D3: Motor Temp * 100 (2 bytes)
    - D4 D5: Battery Voltage * 100 (2 bytes)

### Standard VESC CAN Protocol EID Layout:
- VESC uses: `(Command_ID << 8) | Controller_ID`
  - Bits 8-15: Command ID (e.g., 9 for `CAN_PACKET_STATUS_MSG`, 27 for `CAN_PACKET_STATUS_MSG_5`)
  - Bits 0-7: Controller ID (the CAN ID of the ESC, e.g., 1 for Master, 2 for Slave)

---

## 2. Logic Chain

1. **Reversed EID Bit Mapping**: The dashboard CAN driver is written to extract `mcu_id = (raw_id >> 8) & 0xFF` and `cmd_id = raw_id & 0xFF`. In standard VESC firmware (which runs on these ESCs), these bit positions are reversed: bits 8-15 represent the Command ID, and bits 0-7 represent the Controller ID. Therefore:
   - `mcu_id` in `can_driver.cpp` actually extracts the **VESC Command ID**.
   - `cmd_id` in `can_driver.cpp` actually extracts the **VESC Controller ID (ESC CAN ID)**.

2. **Telemetry Latching Mismatch**:
   - The latching code `if (cmd_id == 0x0B || cmd_id == 0x0C || cmd_id == 0x0D)` checks if the Controller ID is 11 (`0x0B`), 12 (`0x0C`), or 13 (`0x0D`).
   - In standard builds, the Slave ESC is configured with CAN ID 13 (`0x0D`), which matches the `0x0D` condition.
   - When the Slave ESC (ID 13) sends a Status Msg 5 (Command ID 27):
     - `mcu_id` is parsed as 27.
     - `cmd_id` is parsed as 13 (`0x0D`).
     - The latching code sets `slave_id = 27`.
     - The parser targets `slave_esc`.
     - The switch statement checks `cmd_id` (which is `0x0D`). It enters `case 0x0D`.
     - `case 0x0D` parses: `target->voltage = parseI16(&message.data[4]) / 100.0f;`.
     - Since VESC Status Msg 5 stores input voltage at bytes 4-5, it successfully reads the voltage.
     - Consequently, the dashboard displays the battery voltage accurately.

3. **Speed Display Failure**:
   - When the Slave ESC (ID 13) sends a Status Msg 1 (Command ID 9):
     - `mcu_id` is parsed as 9.
     - `cmd_id` is parsed as 13 (`0x0D`).
     - It targets `master_esc` (since `mcu_id` is 9, which matches `master_id` latched to 9 from Master's Status Msg 1).
     - It enters the switch on `cmd_id` = `0x0D`.
     - It parses `target->voltage` from Status Msg 1 (which gets noise/garbage).
     - It **never** enters `case 0x0C` (ERPM & Duty) to parse the ERPM because `cmd_id` is `0x0D`, not `0x0C`.
   - The Master ESC's CAN ID is 1 (standard VESC default) or its CAN broadcast of status messages is disabled because it is in UART control mode.
   - Since no message with `cmd_id = 12 = 0x0C` (which would correspond to Master ESC ID 12) is ever received, `case 0x0C` is never entered.
   - Thus, `erpm` is never updated for either ESC, and `g_vehicle_state.erpm` remains 0, making the speed display always 0.

---

## 3. Caveats

- We assume that the hardware ESCs are configured to run standard VESC firmware (which broadcasts standard status messages on standard EID format). If the ESCs were running custom Flipsky FTESC firmware, both speed and voltage would work because the Slave ESC would broadcast both `0x020C` and `0x020D` and both would be routed correctly. The fact that only voltage works and speed is 0 is a direct mathematical artifact of the VESC status messages being parsed through the reversed custom Flipsky EID mapping.

---

## 4. Conclusion

The root cause of the speed display issue is a mismatch between the dashboard's Flipsky custom CAN layout parser and the standard VESC EID telemetry frames sent by the ESCs. The Slave ESC's CAN ID is 13 (`0x0D`), which maps its Status Msg 5 directly to the voltage parser, but routes its Status Msg 1 (ERPM) away from the speed parser. The Master ESC's messages are ignored because its CAN ID (e.g. 1) does not match any of the Flipsky command codes.

The proposed fix is to update `can_driver.cpp` to correctly parse standard VESC status messages instead of Flipsky's custom protocol (which is accomplished by reversing the EID bit extraction and mapping standard VESC status msg command IDs 9, 16, and 27).

A complete diff patch has been prepared: `c:\Users\thatw\Documents\Apollo-8\DashBoard\.agents\teamwork_preview_explorer_log_exploration_code_audit_2\proposed_can_driver.patch`.

---

## 5. Verification Method

1. Apply the patch `proposed_can_driver.patch` to `src/can_driver.cpp`.
2. Connect the dashboard to the Slave ESC's CAN output and turn on the board.
3. Accelerate the board and verify that the speed displays correctly on the LCD.
4. Verify that the battery voltage and temperature status indicators continue to show accurate data.
