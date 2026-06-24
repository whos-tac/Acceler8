# Handoff Report — Boot Reliability Investigation

## 1. Observation
In the investigation of the boot and UI initialization sequence for the Waveshare Dashboard (`platformio.ini` environment `waveshare_dash`), the following code structures and behaviors were observed:

1. **Display Driver Initialization Code**:
   In `src/display_driver.cpp` (lines 69-96):
   ```cpp
   namespace DisplayDriver {
       void init() {
           Wire.begin(15, 7);
           Wire.beginTransmission(0x24); Wire.write(0x02); Wire.write(0xff); Wire.endTransmission();
           Wire.beginTransmission(0x24); Wire.write(0x03); Wire.write(0x3a); Wire.endTransmission();

           gfx->begin();
           lv_init();

           buf1 = (lv_color_t *)heap_caps_malloc(SCREEN_WIDTH * SCREEN_HEIGHT / 10 * sizeof(lv_color_t), MALLOC_CAP_DMA);
           lv_disp_draw_buf_init(&draw_buf, buf1, NULL, SCREEN_WIDTH * SCREEN_HEIGHT / 10);

           static lv_disp_drv_t disp_drv;
           lv_disp_drv_init(&disp_drv);
           disp_drv.hor_res = SCREEN_WIDTH;
           disp_drv.ver_res = SCREEN_HEIGHT;
           disp_drv.flush_cb = my_disp_flush;
           disp_drv.draw_buf = &draw_buf;
           lv_disp_drv_register(&disp_drv);

           tp.begin();
           tp.setRotation(ROTATION_NORMAL);

           static lv_indev_drv_t indev_drv;
           lv_indev_drv_init(&indev_drv);
           indev_drv.type = LV_INDEV_TYPE_POINTER;
           indev_drv.read_cb = my_touchpad_read;
           lv_indev_drv_register(&indev_drv);
       }
   ```

2. **Hardware Definitions**:
   In `hardware_pinout.md` (lines 44-49):
   ```markdown
   ### Touch Controller (GT911) & IO Expander
   // I2C interface routed to Touch IC and TCA9554PWR (Address 0x24)
   #define TOUCH_SCL_PIN 7
   #define TOUCH_SDA_PIN 15
   ```
   On this board, the TCA9554 expander controls the LCD Reset (`LCD_RST` on Pin 0), Touch Reset (`TP_RST` on Pin 3), and LCD Backlight Enable (`LCD_BL` on Pin 2).

3. **I2C Configuration Byte**:
   In `src/display_driver.cpp` line 72, the expander is configured with `0x3a` (`0011 1010` in binary), making P0 (LCD_RST) and P2 (LCD_BL) outputs, and P3 (TP_RST) an input.

4. **Task Scheduling / Single-threaded Execution**:
   In `src/main.cpp` (lines 13-33), the system runs sequentially in `setup()` and `loop()`. No other concurrent FreeRTOS tasks are spawned. However, the initialization blocks in `CANDriver::init()` with `delay(1500)` and in `UnderglowController::init()` with `delay(500)`.

5. **Build Verification**:
   The `waveshare_dash` environment was verified to compile successfully via PlatformIO:
   ```powershell
   pio run -e waveshare_dash
   ```
   Result: `SUCCESS` (Took 9.92 seconds).

---

## 2. Logic Chain
The reasoning leading to the root cause of the intermittent cold-start blank screen is as follows:

1. **Slow Power Ramping on Cold Starts**:
   Upon power-on (cold start), the power supply rails take time to stabilize. If the ESP32-S3 starts executing code immediately and sends I2C transactions to the TCA9554 IO Expander at address `0x24` without a stabilization delay, the I2C writes will fail silently.

2. **Lack of Active Hardware Reset Pulse**:
   The display controller (ST7701) requires a dedicated hardware reset pulse (pulling RST LOW for at least 10ms, then driving it HIGH and waiting 120ms before sending commands) to exit undefined power-on states. In `src/display_driver.cpp`, `gfx->begin()` is initialized with `GFX_NOT_DEFINED` for the reset pin, meaning the library does not reset the controller. The code never writes to the TCA9554 Output Port register (Register 1) to toggle P0 (LCD_RST) or P3 (TP_RST).

3. **Missing Post-Reset Sleep-Out Delay**:
   Even if the reset pin floated HIGH and the display powered on, the code calls `gfx->begin()` immediately after I2C setup, which begins sending SPI register initialization writes to the ST7701. Without a post-reset delay of at least 120ms (required by ST7701 for internal calibration), these SPI setup commands are ignored by the display controller, causing it to remain unitialized (blank screen).

4. **Immediate Backlight Enable**:
   Because the TCA9554 Output Port register defaults to `0xff` on power-on, setting Pin 2 (LCD_BL) to an output immediately drives it HIGH, turning on the backlight. This explains the symptom where the backlight turns on but the screen remains blank.

5. **Why Warm Starts Work**:
   During warm starts (reset button press or soft reboot), the power rails are already fully stabilized, and the ST7701/TCA9554 chips are already active and in a stable state from the previous run, masking the missing reset sequence.

---

## 3. Caveats
- Since this was a read-only investigation, the proposed changes were not physically run on target hardware.
- The pin mapping of TCA9554 (P0=LCD_RST, P1=TP_INT, P2=LCD_BL, P3=TP_RST) is based on standard Waveshare ESP32-S3-Touch-LCD-4.0 documentation and the existing `0x3a` configuration. If a custom board variant is used, the pin mapping must be verified.

---

## 4. Conclusion
The intermittent blank screen issue on cold starts is a hardware initialization and timing bug caused by:
1. Absence of a power-stabilization boot delay before talking to the I/O expander.
2. Absence of an active hardware reset sequence (toggling reset pins LOW then HIGH).
3. Violation of the ST7701 120ms stabilization timing requirement between reset release and SPI transmission.

### Propose Fix Strategy (under /ponytail mode):
Apply the following minimum changes to `DisplayDriver::init()` in `src/display_driver.cpp`:
- Add a `delay(100)` at the start of `init()` to let power rails stabilize.
- Configure TCA9554 Direction Register to `0x32` to set P3 (TP_RST) as an output as well.
- Manually pull LCD_RST and TP_RST LOW (Output register `0xC0`) for 20ms while keeping the backlight OFF (LCD_BL LOW).
- Release reset (Output register `0xC9`), then wait `delay(120)` to let the screen and touch controller boot up.
- Initialize `gfx->begin()` and `lv_init()`.
- Turn ON the backlight (Output register `0xFF`) once display setup is complete.

The detailed changes are captured in `display_driver_reset.patch` and `proposed_display_driver.cpp` in the agent's folder.

---

## 5. Verification Method
1. **Compilation Check**:
   Run PlatformIO to compile the target environment:
   ```powershell
   pio run -e waveshare_dash
   ```
   Ensure it compiles without errors or warnings.

2. **Physical Validation**:
   Flash the firmware to the device, power down completely (unplug USB/power), wait 10 seconds, and perform a cold start. Verify that the screen starts up reliably 10 out of 10 times.

3. **Invalidation Conditions**:
   If the screen remains black, check if the TCA9554 I2C write returns a failure code (`Wire.endTransmission() != 0`). If it does, increase the initial boot delay or inspect I2C bus wiring/speed.
