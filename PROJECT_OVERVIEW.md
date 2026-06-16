# ACCELER8 Dashboard — Project Overview

> **Read this file first before touching any code.**

This is the firmware for a custom ESK8 (electric skateboard/mountain board) dashboard built on a **Waveshare ESP32-S3-Touch-LCD-4** display module. The project is maintained and developed using **PlatformIO** inside VSCode.

---

## Project Goal

Display real-time telemetry from a **Flipsky FT85BD** dual-ESC setup over a **CAN bus** on a 480×480 pixel touchscreen. The UI is built with **LVGL 8.4.0** and targets a brutalist-minimalist aesthetic (v2.0) centered around high contrast, high readability, and custom pixel-art block fonts.

---

## Hardware Stack

| Component | Model | Notes |
|---|---|---|
| MCU/Display | Waveshare ESP32-S3-Touch-LCD-4 (Rev 4.0) | 480×480 RGB LCD, ST7701 driver |
| ESC | Flipsky FT85BD | VESC-based firmware |
| CAN Transceiver | On-board (connected to TWAI peripheral) | TX: GPIO 6, RX: GPIO 0 |
| Touch IC | GT911 | I2C on SDA=15, SCL=7 |

See `hardware_pinout.md` for all GPIO definitions.

---

## Battery Configuration

- **10S LiPo** pack
- Min voltage: **32.0V** (0%), Max: **42.0V** (100%)
- Percentage is a **linear interpolation** of raw voltage
- Remaining energy (`BATTERY_TOTAL_WH`) is defined in `include/can_driver.h`

---

## Build Environments

The project has **two PlatformIO environments** in `platformio.ini`:

| Environment | Target | Use Case |
|---|---|---|
| `native` | Windows PC (via SDL2/MSYS2) | **UI development & simulation** — no hardware needed |
| `esp32-s3-devkitc-1` | Physical ESP32-S3 board | **Hardware deployment** |

### Running the Native Simulator
```powershell
# MSYS2 must be installed at C:\msys64
$env:PATH += ";C:\msys64\mingw64\bin"
pio run -e native -t exec
```

> **Key rule:** All UI iteration happens in `native` first. **Never flash hardware just to test a label position.**

---

## Source File Structure

```
src/
├── main.cpp              # Entry point. Calls init() and runs the LVGL tick loop.
├── can_driver.cpp        # CAN bus polling + data aggregation
├── espnow_dash.cpp       # ESP-NOW telemetry broadcasting & remote input
├── underglow_controller.cpp # WS2815 LED strip effects logic
├── settings_screen.cpp   # Interactive configuration UI (Gear ratio, Pole pairs, Odometer)
├── ui_controller.cpp     # Main LVGL dashboard widgets and update logic
├── display_driver.cpp    # Low-level SDL2/ST7701 driver init (do not modify)
├── odometer.cpp          # Non-volatile distance tracking
├── espnow_packets.h      # Packet structs for ESP-NOW radio
└── lv_font_block_*.c     # Custom VT323 pixel fonts (24, 56, 72, 300)

include/
├── can_driver.h          # VehicleState struct + g_vehicle_state extern
├── mechanical_config.h   # Wheel diameter, gear ratio, battery Wh constants
├── lv_conf.h             # LVGL feature flags (fonts, widgets enabled here)
└── ...

# Documentation (always keep updated)
vehicle_config.md         # Battery spec, mechanical config status
hardware_pinout.md        # All GPIO pin numbers
can_protocol_dictionary.md # VESC CAN frame parsing reference
.ai_context/              # Persistent knowledge, formulas, and architecture
```

---

## Naming

The project is named **ACCELER8**. Any reference to "Apollo-8" in the filesystem is a legacy folder path and should be treated as synonymous. Never rename the directory structure — only use ACCELER8 in user-visible strings inside the firmware.
