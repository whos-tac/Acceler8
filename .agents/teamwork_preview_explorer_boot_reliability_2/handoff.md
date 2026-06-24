# Handoff Report — Waveshare Dashboard Boot Reliability Investigation

## 1. Observation

1. **Original Initialization Sequence**:
   In `src/display_driver.cpp` (lines 69-74), the display driver initializes the I2C bus and immediately writes configurations to the IO expander at address `0x24`:
   ```cpp
   void init() {
       Wire.begin(15, 7);
       Wire.beginTransmission(0x24); Wire.write(0x02); Wire.write(0xff); Wire.endTransmission();
       Wire.beginTransmission(0x24); Wire.write(0x03); Wire.write(0x3a); Wire.endTransmission();

       gfx->begin();
   ```
   No delays are present prior to or between these I2C transmissions, and their return values are not checked.

2. **IO Expander Configuration (`0x3a`)**:
   In `src/display_driver.cpp` line 72, Configuration register `0x03` is written with `0x3a` (binary `0011 1010` where `0` is output, `1` is input). This configures:
   * Bit 0: Output (0)
   * Bit 1: Input (1)
   * Bit 2: Output (0)
   * Bit 3: Input (1)
   * Bit 4: Input (1)
   * Bit 5: Input (1)
   * Bit 6: Output (0)
   * Bit 7: Output (0)

3. **Schematic Mapping of the IO Expander**:
   Direct extraction of connections from `c:\Users\thatw\Documents\Apollo-8\DashBoard\Resources\Waveshare Board Dash\demo_repo\Schematic\ESP32-S3-Touch-LCD-4 V4.0.pdf` (specifically Page 1, verified in `schematic_text.txt` lines 1810-1835) shows the following connections for the CH32V003 coprocessor `U3` (which mimics TCA9554 at I2C address `0x24`):
   * Pin 0 (`EXIO0`): Labeled `NLEXIO0`, controls P-channel MOSFET `Q4` to switch `VBAT_5V` (active LOW). This rail powers the display backlight boost regulator `U5` (AP3032) and other 5V subsystems.
   * Pin 1 (`EXIO1`): Labeled `NLTP0RST`, connects to the Touch Panel Reset (`TP_RST`) pin of the GT911 touch IC. Active LOW.
   * Pin 2 (`EXIO2`): Labeled `NLTP0INT`, connects to the Touch Panel Interrupt (`TP_INT`) pin of the GT911 touch IC.
   * Pin 3 (`EXIO3`): Labeled `NLLCD0RST`, connects to the LCD Reset (`LCD_RST`) pin of the ST7701 display controller. Active LOW.
   * Pin 6 (`EXIO6`): Labeled `BEE_EN`, controls the active buzzer (active HIGH).

   *Note: This directly invalidates a previous subagent's assumption that Pin 0 was LCD_RST and Pin 3 was TP_RST.*

4. **Build Verification**:
   Running PlatformIO build:
   ```powershell
   pio run -e waveshare_dash
   ```
   Result: `SUCCESS` (compiled in 17.43 seconds).

---

## 2. Logic Chain

1. **Cold Start I2C Failure**:
   On a cold start (powering up the system from 0V), the CH32V003 coprocessor (expander at address `0x24`) requires a startup time (internal reset, oscillator stabilization, bootloader delay) before it becomes responsive on the I2C bus. The ESP32-S3 boots extremely fast and sends I2C transactions to `0x24` immediately upon start. Without a probe delay or retry logic, the I2C writes fail silently with a NACK.

2. **Default Expander State Blocks Power**:
   When the I2C writes fail, the CH32V003 remains in its power-up default configuration where all pins are high-impedance inputs (register 0x03 defaults to `0xff`). P-channel MOSFET `Q4` gate (`EXIO0`) floats HIGH via its 10K pull-up resistor `R22`, keeping `Q4` turned OFF. Consequently, the `VBAT_5V` power rail to the display and backlight controller remains unpowered, resulting in a blank screen.

3. **No Active Hardware Reset Asserted**:
   Even if the I2C configuration succeeds, setting register 0x03 to `0x3a` configures Pin 1 (`TP_RST`) and Pin 3 (`LCD_RST`) as Inputs. They float HIGH via pull-up resistors and are never driven LOW. Consequently, neither the ST7701 display controller nor the GT911 touch controller undergoes an active hardware reset pulse at boot. If either chip starts in an undefined latch-up or noise-induced state, they cannot recover.

4. **Warm Start Success masking the Bug**:
   During warm reboots (soft reset or reset button press), the power supply remains stable and the CH32V003 is already running and responsive, allowing the immediate I2C writes to succeed. Furthermore, the display controller and touch controller are already in a running, initialized state, masking the missing reset sequence.

---

## 3. Caveats

* The investigation was read-only; physical validation on the actual hardware could not be carried out.
* The I2C address `0x24` and TCA9554 register map emulation depend entirely on the pre-programmed firmware of the CH32V003. Any custom modifications to that coprocessor's firmware could alter this behavior, though it matches all production units of Waveshare ESP32-S3-Touch-LCD-4.0.

---

## 4. Conclusion

The intermittent blank screen on cold starts is caused by:
1. Lack of a startup/stabilization delay or I2C polling/retry loop to wait for the CH32V003 coprocessor to boot.
2. Lack of an active hardware reset sequence (driving `LCD_RST` and `TP_RST` LOW, then HIGH) to stabilize the ST7701 display and GT911 touch ICs.
3. Absence of the required post-reset stabilization delay (120ms) before the ESP32-S3 sends SPI commands (`gfx->begin()`).

### Proposed Fix Strategy (under /ponytail mode):

Apply the following modifications to `DisplayDriver::init()` in `src/display_driver.cpp`:

1. Add an I2C probing/wait loop targeting address `0x24` to ensure the coprocessor is ready.
2. Drive all outputs LOW (Output Register = `0x00`) to enable `VBAT_5V` power and hold `LCD_RST` and `TP_RST` LOW.
3. Configure pins 0, 1, 2, 3 as outputs (Configuration Register = `0x30`).
4. Hold the hardware reset LOW for 20ms to allow power rails to stabilize under load and perform a clean reset.
5. Release the resets by driving Pin 1 (`TP_RST`) and Pin 3 (`LCD_RST`) HIGH while keeping Pin 0 (`VBAT_5V` enable) LOW (Output Register = `0x0a`).
6. Wait 120ms to comply with the ST7701 sleep-out calibration time and GT911 start time.
7. Restore original Configuration register (`0x3a`) to return resets to input/pull-up state while keeping `VBAT_5V` output low.

#### Proposed Code Patch:
```cpp
// /ponytail: robust cold start power and reset sequence for Waveshare display
void init() {
    Wire.begin(15, 7);

    // Wait for the CH32V003 coprocessor (IO expander) to power up on the I2C bus
    int retry = 15;
    bool expander_ready = false;
    while (retry > 0) {
        Wire.beginTransmission(0x24);
        if (Wire.endTransmission() == 0) {
            expander_ready = true;
            break;
        }
        delay(10);
        retry--;
    }

    if (expander_ready) {
        // Set polarity inversion register (standard for this board)
        Wire.beginTransmission(0x24); Wire.write(0x02); Wire.write(0xff); Wire.endTransmission();

        // 1. Drive all outputs LOW:
        //    Pin 0 (VBAT_5V Enable) = LOW (enables P-MOSFET Q4 to power display/backlight)
        //    Pin 1 (TP_RST) = LOW (assert touch reset)
        //    Pin 2 (TP_INT) = LOW (holds touch INT low for GT911 I2C address selection)
        //    Pin 3 (LCD_RST) = LOW (assert LCD reset)
        Wire.beginTransmission(0x24); Wire.write(0x01); Wire.write(0x00); Wire.endTransmission();

        // 2. Set Configuration register: set Pins 0, 1, 2, 3, 6, 7 as outputs (Configuration = 0x30)
        Wire.beginTransmission(0x24); Wire.write(0x03); Wire.write(0x30); Wire.endTransmission();

        // Hold resets LOW for 20ms to stabilize power rails and reset display/touch controllers
        delay(20);

        // 3. Release resets:
        //    Pin 0 (VBAT_5V Enable) = LOW (remain ON)
        //    Pin 1 (TP_RST) = HIGH (release touch reset)
        //    Pin 2 (TP_INT) = LOW (GT911 requires INT to be held low during reset release)
        //    Pin 3 (LCD_RST) = HIGH (release LCD reset)
        //    Output register value: binary 0000 1010 = 0x0a
        Wire.beginTransmission(0x24); Wire.write(0x01); Wire.write(0x0a); Wire.endTransmission();

        // 4. Wait 120ms to allow ST7701 (sleep-out) and GT911 (startup) internal calibration to finish
        delay(120);

        // 5. Restore vendor pin direction configuration:
        //    Pin 1 and Pin 3 back to Inputs (they float HIGH via hardware pull-up resistors)
        //    Configuration register value: 0x3a
        Wire.beginTransmission(0x24); Wire.write(0x03); Wire.write(0x3a); Wire.endTransmission();
    }

    gfx->begin();
    lv_init();
    // ... rest of driver initialization ...
```

---

## 5. Verification Method

1. **Compilation Verification**:
   Execute the PlatformIO compile command:
   ```powershell
   pio run -e waveshare_dash
   ```
   Ensure the code compiles without warnings or errors.

2. **Cold Start Verification**:
   Flash the modified code to the board. Disconnect power completely for at least 15 seconds to discharge capacitors. Re-apply power (cold start) and verify that the screen and backlight initialize and display the UI instantly. Repeat 10 times to ensure reliability.

3. **Telemetry Logging**:
   Monitor the Serial console at 115200 baud. If the expander initialization fails or times out, print a diagnostic message to assist in hardware troubleshooting:
   ```cpp
   if (!expander_ready) {
       Serial.println("[ERROR] CH32V003 Coprocessor not responding on I2C address 0x24!");
   }
   ```
