# CAN Protocol Dictionary

Telemetry mappings based on the VESC protocol standard for the Flipsky ESC.

## Message Format
- **ID Type**: 29-bit Extended ID (EID)
- **Byte Order**: Big-Endian
- **ID Structure**: Bits 0-7 = MCU ID, Bits 8-15 = CAN Command ID

## Messages

### STATUS_1 (0x09)
*   **Bytes 0-3** (Int32): ERPM
*   **Bytes 4-5** (Int16): Current (x10)
*   **Bytes 6-7** (Int16): Duty Cycle (x1000)

### STATUS_4 (0x10)
*   **Bytes 0-1** (Int16): MOSFET Temp (x10)
*   **Bytes 2-3** (Int16): Motor Temp (x10)
*   **Bytes 4-5** (Int16): Battery Current In (x10)
*   **Bytes 6-7** (Int16): Rotor Position (x50)

### STATUS_5 (0x1B)
*   **Bytes 0-3** (Int32): Tachometer
*   **Bytes 4-5** (Int16): Battery Voltage In (x10)
