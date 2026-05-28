# CAN Bus Implementation: Flipsky FTESC Edition

## Physical Layer
- **Interface**: ESP32-S3 TWAI Peripheral.
- **Pins**: GPIO 6 (TX), GPIO 0 (RX).
- **Bitrate**: **1000 kbps (1 Mbps)**.
- **Transceiver**: Internal ESP32-S3.

## Protocol Detail (Flipsky Custom)
The Flipsky FT85BD uses a non-standard VESC-lite EID format:
- **MCU ID**: Bits 8-15.
- **CMD ID**: Bits 0-7.

### Device IDs
- **Master ESC**: 163 (0xA3)
- **Slave ESC**: 224 (0xE0)

### Message Structure
| CMD ID | Name | Data Layout |
|---|---|---|
| `0x0B` | Power | Bytes 0-2 (I24): Motor A. Bytes 3-5 (I24): Batt A. |
| `0x0C` | Velocity| Bytes 0-2 (I24): ERPM. Bytes 3-5 (I24): Duty Cycle. |
| `0x0D` | Health | Bytes 0-1 (I16): FET Temp. Bytes 2-3 (I16): Motor Temp. Bytes 4-5 (I16): Voltage. |

## Data Aggregation Logic
The `CANDriver::poll()` function aggregates data from both ESCs into a single `g_vehicle_state`:
1. **Voltage**: Averaged between master and slave.
2. **Current**: Summed (`Master Batt A + Slave Batt A`) to represent total system load.
3. **ERPM**: Derived from the Master (or Slave if Master is dead) for speed calculation.
4. **Temps**: `MAX(Master, Slave)` is used to ensure the hottest component is monitored.
5. **Tachometer**: Incremented based on Master ERPM.

## Formulas & Scaling
- **I24 Sign Extension**: 
  `if (val & 0x800000) val |= 0xFF000000;`
- **Temp/Volt Scale**: Value / 100.0f.
- **Current Scale**: Value / 1000.0f.
- **Speed (km/h)**: Derived from ERPM using Wheel Diameter and Gear Ratio (defined in `mechanical_config.h`).
