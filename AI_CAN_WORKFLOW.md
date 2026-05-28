# AI Workflow: CAN Bus & Data Pipeline (Flipsky FTESC Edition)

This document describes the current CAN implementation specifically for the Flipsky FT85BD dual-ESC setup.

---

## Architecture Overview

```
[Master ID 163]      [Slave ID 224]
       │                    │
       └──────────┬─────────┘
                  ▼
         CAN Bus (1000k bps)
       (ESC_ID << 8 | CmdID)
                  ▼
        [CANDriver::poll()]
      (Sums Amps, Max Temps)
                  ▼
          [g_vehicle_state]
                  ▼
       [UIController::update()]
```

---

## The Flipsky Protocol (V1.4)

Unlike standard VESC, the Flipsky FTESC uses a modified 29-bit Extended ID layout and different scaling factors.

### ID Decoding
- **Bits 0-7**: COMMAND ID
- **Bits 8-15**: MCU (ESC) ID
- **Bits 16-28**: Reserved

### Supported Frames

#### Data 0 — `cmd_id = 0x0B` (11)
- **Bytes 0-2 (I24 BE)**: Motor Current (A) × 1000
- **Bytes 3-5 (I24 BE)**: Battery Current (A) × 1000

#### Data 1 — `cmd_id = 0x0C` (12)
- **Bytes 0-2 (I24 BE)**: Motor ERPM
- **Bytes 3-5 (I24 BE)**: Duty Cycle × 1000

#### Data 2 — `cmd_id = 0x0D` (13)
- **Bytes 0-1 (I16 BE)**: MOSFET Temp (°C) × 100
- **Bytes 2-3 (I16 BE)**: Motor Temp (°C) × 100
- **Bytes 4-5 (I16 BE)**: Battery Voltage (V) × 100

---

## Implementation Details

### Aggregation Logic
The `CANDriver` maintains internal state for both ESCs:
- **Voltage**: Averaged between both ESCs when available.
- **Battery Amps**: Summed (`Master Amps + Slave Amps`) to show total power usage in Watts.
- **Temps**: Maximum value across both ESCs is displayed for safety monitoring.
- **Speed**: Derived from Master ERPM by default.

### Derived Metrics
- `power_w`: Total Watts (Total Amps × Avg Voltage).
- `speed_kmh`: Based on ERPM, Motor Pole Pairs (7), and Gear Ratio (from mechanical_config.h).

---

## CAN Configuration
- **Hardware Pins**: GPIO 6 (TX), GPIO 0 (RX).
- **Speed**: 1000 kbps (1 Mbps).
- **Transceiver**: Built-in ESP32-S3 TWAI driver.
