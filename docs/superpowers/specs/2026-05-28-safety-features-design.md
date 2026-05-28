# Safety Features & Global Alert System Design

## Overview
This document outlines the design for integrating critical safety features inspired by `gb_remote` into the ACCELER8 DashBoard. The goal is to provide a robust, brutalist-compliant alert system that handles connection loss, hardware faults, and battery safety proactively.

## Architecture & Data Flow

### 1. Connection Health Tracking
To ensure the dashboard never displays stale or "frozen" data (which can be dangerous for a rider):
- **Staleness Tracking**: A `last_update_timestamp` will be added to the `EscData` struct for each ESC in `g_vehicle_state`. If the time since the last heartbeat exceeds 500ms, the system will flag a "CAN Timeout / Disconnect" state.
- **RTOS Watchdog**: The CAN driver task will be registered with the ESP32 `esp_task_wdt`. If the task hangs or fails to process the queue, the watchdog will reset the system or trigger an immediate fail-safe state.

### 2. Fault Detection Triggers
The system will monitor the following metrics from the Flipsky FT85BD CAN stream to trigger safety alerts:
- **Pack Voltage Sag**: Low voltage thresholds to warn before ESC low-voltage cutoffs engage.
- **Thermal Limits**: ESC and Motor Overtemp thresholds (leveraging the aggregated max temps in `g_vehicle_state`).
- **ESC Fault Codes**: Native fault codes reported over the Flipsky CAN protocol (if available).

## Presentation Layer (UI)

### Full-Screen Modal Overlay
To ensure the rider immediately notices a critical safety issue without cluttering the main brutalist dashboard:
- **Visual Design**: A full-screen modal overlay that completely hides the standard telemetry (Hero Row, Gauges, Stats).
- **Aesthetic**: High contrast (e.g., solid Red or flashing Red/Black background) with large, pixel-art typography (VT323/UNSCII) describing the exact error (e.g., "ERR: CAN TIMEOUT", "WARN: MOTOR OVERTEMP").
- **Interaction**: The modal will persist until the error condition clears (e.g., connection restored, temps drop below the threshold) or the user explicitly dismisses it (if a button interface is available).

## Implementation Path
1. Update `VehicleState` and `EscData` structs to include timestamps and fault flags.
2. Modify `CANDriver` to update timestamps on message receipt and initialize the RTOS watchdog.
3. Build the `AlertOverlay` LVGL component in `UIController` that subscribes to the global fault state and overlays the active screen.

## Additional Features (Remote vs. Dashboard Split)
To ensure system boundaries are respected, the remaining `gb_remote` features are split between the Lilygo Remote (input/haptics/sleep) and the Waveshare Dash (display/storage/settings).

### Lilygo Remote (ESP32 + 5 Buttons + Potentiometer)
1. **Throttle Calibration UI Trigger & Mapping**: Reads the potentiometer. When placed in calibration mode via specific button presses, maps raw ADC limits to 0-100% and stores them in NVS. Sends normalized 0-100% values to the Dash/Receiver.
2. **Inactivity Sleep Management**: Monitors potentiometer and button activity. If idle for X minutes, puts the remote into deep sleep.
3. **Haptic/Audible Feedback**: If a vibration motor or buzzer is connected, pulses on connection loss, low remote battery, or critical Dash alerts.
4. **Remote Battery Monitoring**: Monitors its own battery voltage and sends it to the Dash.
5. **Button Mapping**:
   - `Btn 1` (Deadman/Power)
   - `Btn 2` (Mode up/down)
   - `Btn 3` (Settings menu trigger)
   - `Btn 4 & 5` (UI Navigation)

### Waveshare Dash (ESP32-S3 + Display)
1. **Persistent Odometer**: Tracks distance based on ERPM/Speed and periodically saves it to NVS storage to survive reboots.
2. **Dual Battery Displays**: Displays the vehicle's main battery (from CAN) alongside the Remote's battery level (received over UART/BLE).
3. **Automatic Parameter Sync**: Fetches required constants (Motor Poles, Wheel Diameter, Gearing) from the Flipsky ESC via CAN to automatically calculate accurate speed/odometer values.
4. **Settings & Configuration UI**: A dedicated LVGL menu screen (navigated via the Remote's buttons) to toggle speed units (MPH/KMH), adjust screen brightness, and trigger remote throttle calibration.
