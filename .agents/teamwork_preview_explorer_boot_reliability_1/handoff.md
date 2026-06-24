# Handoff Report — Boot and UI Initialization Reliability Fix

## 1. Observation
We observed the following code structures and behaviors:

1. In `src/display_driver.cpp` (lines 70-74), the initialization sequence for the I2C expander is:
```cpp
        Wire.begin(15, 7);
        Wire.beginTransmission(0x24); Wire.write(0x02); Wire.write(0xff); Wire.endTransmission();
        Wire.beginTransmission(0x24); Wire.write(0x03); Wire.write(0x3a); Wire.endTransmission();

        gfx->begin();
```
   There is no write to Register `0x01` (Output Port register) of the TCA9554 IO expander, nor are there any hardware reset delays or stabilization delays before beginning the display communication.

2. In `hardware_pinout.md` (lines 44-49):
```markdown
### Touch Controller (GT911) & IO Expander
// I2C interface routed to Touch IC and TCA9554PWR (Address 0x24)
#define TOUCH_SCL_PIN 7
#define TOUCH_SDA_PIN 15
```

3. In `src/display_driver.cpp` (lines 34-36), the display is constructed with `GFX_NOT_DEFINED` for its hardware reset pin:
```cpp
Arduino_RGB_Display *gfx = new Arduino_RGB_Display(
  SCREEN_WIDTH, SCREEN_HEIGHT, rgbpanel, 0 /* rotation */, true /* auto_flush */,
  bus, GFX_NOT_DEFINED /* RST */, st7701_type1_init_operations, sizeof(st7701_type1_init_operations));
```

4. In `src/display_driver.cpp` (lines 14-21), the touch controller is also instantiated with `-1` for the reset pin:
```cpp
// Touch IC Configuration
#define TOUCH_SDA  15
#define TOUCH_SCL  7
#define TOUCH_INT  -1
#define TOUCH_RST  -1
// ...
TAMC_GT911 tp = TAMC_GT911(TOUCH_SDA, TOUCH_SCL, TOUCH_INT, TOUCH_RST, TOUCH_WIDTH, TOUCH_HEIGHT);
```

5. In `.pio/libdeps/waveshare_dash/TAMC_GT911/TAMC_GT911.cpp` (lines 15-27), the `reset()` method checks and manipulates reset pins. However, because `TOUCH_RST` is `-1`, the call to `pinMode(-1, OUTPUT)` and `digitalWrite(-1, ...)` does nothing on ESP32 Arduino.

6. PlatformIO build command `pio run -e waveshare_dash` compiles successfully:
```
RAM:   [===       ]  29.1% (used 95508 bytes from 327680 bytes)
Flash: [===       ]  33.2% (used 1110721 bytes from 3342336 bytes)
========================= [SUCCESS] Took 9.85 seconds =========================
```

## 2. Logic Chain
1. **Reset Pin Assignment**: On the Waveshare ESP32-S3-Touch-LCD-4 board, the hardware reset lines for both the ST7701 display controller and the GT911 touch IC are connected to the TCA9554 IO Expander (address `0x24`), specifically at pin P0 (GT911 RST) and pin P2 (ST7701 RST).
2. **Missing Reset Sequence**: Because the display and touch libraries receive `GFX_NOT_DEFINED` (`-1`) for their reset pins, they cannot perform a hardware reset directly. The application code in `display_driver.cpp` sets the pin directions on the TCA9554 (Register 3) but never writes to Register 1 (Output Port) to drive the reset pins.
3. **Power-On Default**: On a cold start, the TCA9554 defaults to setting all its output pins high (`0xff`). Thus, the reset lines are pulled high upon power-up and are never toggled or held low to reset the display controller or touch controller.
4. **Race Condition / Lack of Power-Up Delay**: On cold starts, the rising power supply ramp can cause the ESP32 to boot up and begin sending SPI configuration commands (via `gfx->begin()`) before the ST7701 display controller has finished its internal Power-On Reset (POR) cycle.
5. **Dropped Initialization Commands**: When `gfx->begin()` is called immediately without an explicit reset pulse and a subsequent recovery delay, the ST7701 ignores the initial setup commands. This leaves the display in sleep mode with a blank screen.
6. **Warm Start Stability**: On a warm start (hot reboot), the power rails are already fully stable, and the display controller is already booted, which explains why it responds to the initialization commands and works fine intermittently.

## 3. Caveats
- We assumed the standard pinout for Waveshare ESP32-S3-Touch-LCD-4 (Rev 4.0), where P0 is GT911 RST and P2 is ST7701 RST.
- We did not investigate potential hardware defects (e.g. faulty hardware pull-ups or power supply noise).
- The solution relies on the standard recovery delay of 120ms required by the ST7701 display driver datasheet after releasing reset.

## 4. Conclusion
The intermittent blank screen on cold starts is caused by a missing hardware reset toggle sequence and the lack of a startup recovery delay (120ms) before sending display commands.
Implementing a proper reset sequence on the TCA9554 IO Expander (pulling P0/P2 low for 20ms, then driving them high, and waiting 120ms) will resolve the issue.

A patch file (`boot_reset_fix.patch`) has been generated in the agent's folder containing the proposed fix for `src/display_driver.cpp` following the `/ponytail` minimalist code guidelines.

## 5. Verification Method
1. Inspect the patch file `boot_reset_fix.patch` in the explorer agent's folder.
2. Confirm the project compiles after applying the patch using:
   ```powershell
   pio run -e waveshare_dash
   ```
3. To invalidate the solution, boot the physical Waveshare Dashboard from a completely powered-down state (cold start) multiple times (e.g., 20 cycles) and check if the display fails to turn on.
