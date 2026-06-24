# Project: Boot Reliability Fix

## Architecture
- Target Platform: Waveshare Dashboard (compiled via platformio env `waveshare_dash`)
- Display driver, LVGL setup, I2C/SPI bus initialization, and FreeRTOS task scheduling.

## Milestones
| # | Name | Scope | Dependencies | Status | Conversation ID |
|---|------|-------|-------------|--------|-----------------|
| 1 | Investigate and Fix Boot Rendering | Analyze initialization sequence, identify race/readiness issues, implement boot delays/task/bus adjustments, compile with waveshare_dash, and verify. | None | DONE | 167c9ebf-59e2-4553-b1ed-f1d29a6c8f26, 291b4c10-1767-420e-9fa7-51f987250121, 92dfc6ca-1fa3-4b8d-b533-7a1a52629ecb |

## Interface Contracts
### TCA9554 IO Expander Pin Assignments (Address 0x24)
- **Pin 0 (EXIO0)**: `VBAT_5V` power rail switch enable. Active LOW. Enables P-channel MOSFET `Q4` to power display/backlight.
- **Pin 1 (EXIO1)**: Touch Panel Reset (`TP_RST`) of GT911 touch IC. Active LOW.
- **Pin 2 (EXIO2)**: Touch Panel Interrupt (`TP_INT`) of GT911 touch IC.
- **Pin 3 (EXIO3)**: LCD Reset (`LCD_RST`) of ST7701 display controller. Active LOW.
- **Pin 6 (EXIO6)**: Buzzer Enable (`BEE_EN`). Active HIGH.

### Boot Sequence Requirements
1. **Expander Handshake**: Probe I2C address `0x24` with retry logic (up to 15 retries at 10ms intervals) to wait for CH32V003 bootloader completion.
2. **Power-On Reset**: Configure Pins 0, 1, 2, 3 as outputs (write `0x30` to Configuration Register `0x03`).
3. **Assert Reset**: Drive all outputs LOW (write `0x00` to Output Register `0x01`). This turns on `VBAT_5V` and holds LCD and Touch controllers in hardware reset. Wait 20ms.
4. **Release Reset**: Drive Pin 1 (`TP_RST`) and Pin 3 (`LCD_RST`) HIGH while keeping Pin 0 and Pin 2 LOW (write `0x0a` to Output Register `0x01`). Wait 120ms for display sleep-out POR and touch IC calibration.
5. **Restore Pin Directions**: Set Pin 1 and Pin 3 back to inputs (write `0x3a` to Configuration Register `0x03`) to allow hardware pull-ups to control them, preserving default behavior.

## Code Layout
- `src/display_driver.cpp` — display and touchscreen peripheral initialization and setup sequence.
