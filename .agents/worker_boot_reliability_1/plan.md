# Plan — Boot Rendering Reliability Fix Implementation

## Objective
Implement the boot rendering reliability fix in `src/display_driver.cpp` as requested.

## Requirements & Implementation Steps
1. **Startup Delay**: Add a 100ms startup delay (`delay(100);`) at the very beginning of `DisplayDriver::init()` to let the power rails stabilize on cold boot.
2. **I2C Expander Configuration**:
   - Keep `Wire.begin(15, 7);` to initialize I2C.
   - Add a brief settlement delay of 10ms after I2C initialization (from Explorer 3 patch, helps I2C bus settle).
   - Write `0xFF` to polarity inversion register (0x02).
   - Write `0x32` to the Direction Register (0x03) to configure P0 (LCD_RST), P2 (LCD_BL), and P3 (TP_RST) as outputs.
   - Include a `// ponytail:` comment explaining this configuration.
3. **Pulse Resets**:
   - Pull resets low: Write `0xC0` to Output Register (0x01) to pull P0 (LCD_RST) and P3 (TP_RST) low, keeping P2 (LCD_BL) low.
   - Wait 20ms (`delay(20);`).
   - Release resets: Write `0xC9` to Output Register (0x01) to release resets (driving P0/P3 high, keeping P2 low).
4. **Post-Reset Delay**: Wait 120ms (`delay(120);`) for the LCD internal POR/calibration.
5. **Backlight Enable**:
   - Perform display initialization (`gfx->begin()`, `lv_init()`, drawing buffer config, etc.).
   - Perform touch initialization (`tp.begin()`, set rotation).
   - Write `0xFF` to Output Register (0x01) to turn on the backlight (LCD_BL = P2 HIGH).
6. **Compile Verification**: Run `pio run -e waveshare_dash` to check that the build succeeds.
7. **Handoff Documentation**: Write `handoff.md` and notify the orchestrator.
