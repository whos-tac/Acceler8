# Deep Context: Flipsky FTESC CAN Protocol V1.4

## Protocol Identification
The Flipsky FTESC (specifically the FT85BD and similar series) uses a 29-bit Extended CAN Identifier that differs from the VESC standard used in older firmware.

### ID Structure
```
[Bits 28-16] : Reserved / 0
[Bits 15-8]  : ESC ID (MCU ID)
[Bits 7-0]   : Command ID
```

## Parsing Matrix

| Command | Name | Field | Bytes | Type | Scaling |
|---|---|---|---|---|---|
| **0x0B** | DATA_0 | Motor Current | 0-2 | I24 BE | / 1000.0 |
| | | Battery Current | 3-5 | I24 BE | / 1000.0 |
| **0x0C** | DATA_1 | ERPM | 0-2 | I24 BE | / 1.0 |
| | | Duty Cycle | 3-5 | I24 BE | / 1000.0 |
| **0x0D** | DATA_2 | FET Temp | 0-1 | I16 BE | / 100.0 |
| | | Motor Temp | 2-3 | I16 BE | / 100.0 |
| | | Voltage | 4-5 | I16 BE | / 100.0 |

## Big-Endian Logic (I24)
Unlike standard VESC which often uses I32, Flipsky uses 3-byte integers for high precision.
```cpp
int32_t val = (data[0] << 16) | (data[1] << 8) | data[2];
if (val & 0x800000) val |= 0xFF000000; // Sign Extension
```

## Known Hardware
- **Master**: ID 163
- **Slave**: ID 224
- **Baud**: 1000k

## Source Document
See `Resources/FLIPSKY FTESC CAN Protocol V1.4 English.pdf` for full command set including gear shifting and throttle control.
