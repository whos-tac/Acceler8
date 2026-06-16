# ACCELER8 (Apollo-8) Dashboard

> A high-performance, real-time telemetry dashboard and control ecosystem for electric skateboards and mountain boards (ESK8/MTB).

![Project Status](https://img.shields.io/badge/Status-Active-success) ![PlatformIO](https://img.shields.io/badge/PlatformIO-Compatible-orange) ![UI](https://img.shields.io/badge/UI-LVGL%208.4-blue)

ACCELER8 is a custom-built C++ firmware designed for the **Waveshare ESP32-S3-Touch-LCD-4**. It replaces rudimentary battery indicators with a deeply integrated, 60 FPS graphical interface that pulls granular, high-speed telemetry directly from the motor controllers via CAN bus.

## 🚀 Key Features

- **Brutalist "Glanceable" UI**: Built with LVGL 8.4.0, featuring a high-contrast aesthetic, custom VT323 pixel-art typography, and dynamic thermo-dynamic/power bars that change color during regenerative braking.
- **Real-Time CAN Telemetry**: Deep integration with the **Flipsky FT85BD Dual-FTESC** via the onboard TWAI (CAN) peripheral. Aggregates Master and Slave FTESC data to provide total power output, average voltage sag, and peak temperatures.
- **3-Node ESP-NOW Network**: Microsecond-latency, connectionless wireless communication between:
  1. The **Dashboard** (Telemetry Aggregator & Visual Hub)
  2. The **Receiver** (ESP8266 D1 Mini - Acts as a UART bridge to the FTESC)
  3. The **Remote** (Lilygo T-Display S3 - Transmits throttle/buttons and displays battery voltage on the hand controller)
- **Interactive Remote-Navigated Settings**: No touch screen required. Use the remote control's D-pad to configure mechanical settings on the fly (Pole Pairs, Gear Ratio, Wheel Diameter). Settings persist across reboots via the ESP32 `Preferences` library.
- **Dynamic WS2815 Underglow**: Speed-reactive and breathing LED animations driven by the hardware I2S DMA controller to prevent CPU starvation and signal flickering.
- **Peripheral Control**: Remote-triggered Horn and Headlights via the receiver's UART outputs, including an automatic braking light when the throttle is pulled backward.

## 🧰 Hardware Stack

| Component | Model | Function |
|---|---|---|
| **Dashboard / MCU** | Waveshare ESP32-S3-Touch-LCD-4 | Main UI, CAN telemetry parser, ESP-NOW Broadcaster |
| **Motor Controller** | Flipsky FT85BD Dual-FTESC | Drive system, CAN Telemetry source |
| **Remote Control** | Lilygo T-Display S3 | User input (Throttle, D-pad), Battery UI |
| **Receiver Node** | ESP8266 D1 Mini | Bridges remote inputs to FTESC via UART |
| **LED Underglow** | WS2815 (12V) | Deck lighting via `NeoPixelBus` |

## 🏗️ Architecture & Protocols

- **CAN Bus (1 Mbit/s)**: Parses the Flipsky V1.4 Extended Identifier (29-bit EID) protocol. Aggregates data from Master (ID 163) and Slave (ID 224). *Note: This project strictly interfaces with FTESC architectures.*
- **ESP-NOW**: 2.4GHz WiFi link operating on Channel 1. Handles `ControlPacket` (10Hz throttle & bitmasked button states), `TelemetryPacket` (Dashboard to Remote), and `EscConfigPacket`.
- **FreeRTOS**: Separates the heavy LVGL graphics rendering task from the time-critical CAN polling and ESP-NOW interrupts.

## 💻 Development & Compilation

The project is maintained using **PlatformIO** inside VSCode and features dual compilation targets to enforce a "Simulation-First" workflow.

### Environments (`platformio.ini`)

1. `native` (PC Simulator)
   - **Target:** Windows/SDL2 via MSYS2
   - **Use Case:** Rapid UI/UX iteration and development. Runs a synthetic telemetry engine to mock CAN data. *Never flash physical hardware just to test a UI layout.*
   
2. `esp32-s3-devkitc-1` (Hardware Target)
   - **Target:** Waveshare ESP32-S3
   - **Use Case:** Production deployment to the physical dashboard.

### Quick Start
```bash
# Clone the repository
git clone https://github.com/yourusername/ACCELER8.git
cd ACCELER8

# To run the UI simulator (Requires MSYS2 in C:\msys64)
pio run -e native -t exec

# To flash the dashboard hardware
pio run -e esp32-s3-devkitc-1 -t upload
```

## 📂 Project Structure

- `src/ui_controller.cpp`: LVGL widgets, UI layout, and dynamic data binding.
- `src/can_driver.cpp`: TWAI hardware initialization, EID parsing, and Dual-FTESC data aggregation.
- `src/espnow_dash.cpp`: Radio transmission logic and peer-to-peer MAC management.
- `src/settings_screen.cpp`: Non-volatile configuration menus.
- `include/vehicle_config.h`: Core mechanical and electrical constraints.
- `.ai_context/`: Contains deep protocol reverse-engineering documentation. (Keep this updated!)
