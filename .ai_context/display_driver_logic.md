# Display & Driver Logic

## Hardware Target
- **Board**: Waveshare ESP32-S3-Touch-LCD-4 (Revision 4.0).
- **Display Driver**: ST7701 (RGB Interface).
- **Resolution**: 480x480.
- **Touch Controller**: GT911 (I2C at Address 0x14/0x5D).

## Environment Configuration (`platformio.ini`)
- **PSRAM**: Required for the RGB buffer.
- **LCD_DE/VSYNC/HSYNC/PCLK**: Precise timing is critical for the ST7701. See `hardware_pinout.md` for specific GPIOs.
- **Baud Rate**: `monitor_speed = 115200` for Serial debgging.

## Simulation Layer (Native)
The `[env:native]` environment uses **SDL2** to render the LVGL buffer on a PC window.
- **Dependencies**: MSYS2 (Mingw-w64).
- **Path**: `C:\msys64\mingw64\bin` must be in the environment PATH.
- **Scaling**: The simulator window is pixel-perfect to the 480x480 hardware target.

## Critical Build Flags
- `LV_CONF_INCLUDE_SIMPLE`: Simplifies standard LVGL config inclusion.
- `LV_LVGL_H_INCLUDE_SIMPLE`: Ensures consistent header paths between Native and ESP32 builds.
