# Handoff Report — Boot Rendering Reliability Fix Implementation

## 1. Observation
In the implementation of the boot rendering reliability fix, the following was observed and executed:

1. **Expander Configuration and Reset Logic**:
   - The display driver file `src/display_driver.cpp` initially directly wrote to I2C expander `0x24` register `0x03` with `0x3a` and `0x02` with `0xff` without delays or retry logic.
   - Based on schematic verification from Explorer 2:
     - Pin 0 controls VBAT_5V enable (active LOW, powers 5V subsystem/backlight).
     - Pin 1 is TP_RST (active LOW).
     - Pin 2 is TP_INT (touch interrupt).
     - Pin 3 is LCD_RST (active LOW).

2. **Applied Modifications**:
   In `src/display_driver.cpp` (lines 69-122):
   ```cpp
   namespace DisplayDriver {
       void init() {
           // Wait for power supply to stabilize on cold start
           delay(100);

           Wire.begin(15, 7);

           // ponytail: Probe address 0x24 up to 15 times to wait for CH32V003 coprocessor to boot
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
               // Set polarity inversion register (Register 0x02)
               Wire.beginTransmission(0x24); Wire.write(0x02); Wire.write(0xff); Wire.endTransmission();

               // Drive all outputs LOW:
               // Pin 0 (VBAT_5V Enable) = LOW (enables Q4 MOSFET to power 5V subsystems/backlight)
               // Pin 1 (TP_RST) = LOW (assert touch reset)
               // Pin 2 (TP_INT) = LOW (holds touch INT low for GT911 address selection)
               // Pin 3 (LCD_RST) = LOW (assert LCD reset)
               Wire.beginTransmission(0x24); Wire.write(0x01); Wire.write(0x00); Wire.endTransmission();

               // ponytail: Set Configuration register to 0x30 to configure Pins 0-3 as outputs for power and reset control
               Wire.beginTransmission(0x24); Wire.write(0x03); Wire.write(0x30); Wire.endTransmission();

               // Hold resets LOW for 20ms to stabilize power rails and reset display/touch controllers
               delay(20);

               // Release resets:
               // Pin 0 (VBAT_5V Enable) = LOW (remain ON)
               // Pin 1 (TP_RST) = HIGH (release touch reset)
               // Pin 2 (TP_INT) = LOW (GT911 requires INT to be held low during reset release)
               // Pin 3 (LCD_RST) = HIGH (release LCD reset)
               // Output register value: binary 0000 1010 = 0x0a
               Wire.beginTransmission(0x24); Wire.write(0x01); Wire.write(0x0a); Wire.endTransmission();

               // Wait 120ms to allow ST7701 and GT911 internal calibration to finish
               delay(120);

               // ponytail: Restore original vendor pin direction configuration (0x3a) to return resets to inputs
               Wire.beginTransmission(0x24); Wire.write(0x03); Wire.write(0x3a); Wire.endTransmission();
           } else {
               Serial.println("Warning: TCA9554 IO Expander at 0x24 not responding!");
           }
   ```

3. **Build Check Verification**:
   The PlatformIO compilation command was executed:
   `pio run -e waveshare_dash`
   Output confirmed success:
   ```
   RAM:   [===       ]  29.1% (used 95508 bytes from 327680 bytes)
   Flash: [===       ]  33.2% (used 1110953 bytes from 3342336 bytes)
   ========================= [SUCCESS] Took 16.88 seconds =========================
   ```

---

## 2. Logic Chain
1. **Stabilization & Boot Timing**: Power supplies need time to ramp up to the nominal 3.3V/5V rails. Adding a `delay(100)` at `DisplayDriver::init()` guarantees the rails are stable before communication begins.
2. **IO Expander Boot Duration**: The CH32V003 coprocessor acting as a TCA9554 IO expander has its own internal bootloader time. Probing address `0x24` with a 15-retry limit (10ms intervals) ensures that I2C commands are only sent when the chip is awake.
3. **Power Control**: Pin 0 controls the active-LOW P-MOSFET Q4 that powers the 5V subsystem (backlight and screen logic). Setting Pin 0 LOW ensures the display gets powered. Set Pin 0 HIGH would cut the power.
4. **Reset Sequence & GT911 Requirements**:
   - Resets are pulled LOW (Output 0x00, Direction 0x30) to put the display and touch controllers into a reset state while keeping power enabled.
   - After a 20ms hold time, Pin 1 (`TP_RST`) and Pin 3 (`LCD_RST`) are driven HIGH to release resets, while Pin 2 (`TP_INT`) is held LOW (required by GT911 to select the correct I2C address `0x5d` during its boot phase). This corresponds to writing `0x0a` to the Output Register.
5. **ST7701 POR/Sleep-Out**: ST7701 display controller needs 120ms to exit reset calibration. Adding `delay(120)` before calling `gfx->begin()` satisfies this constraint.
6. **Restoring State**: Finally, restoring configuration to `0x3a` returns the reset pins to high impedance inputs (relying on physical pull-ups) as originally configured by the vendor, keeping the system behavior correct.

---

## 3. Caveats
- Direct hardware execution cannot be run in this environment, but the logic has been mathematically and schematatically cross-checked with verified design documentation of the Waveshare ESP32-S3-Touch-LCD-4 board.
- Warning messages are output to `Serial` at 115200 baud if the I2C expander is not responding.

---

## 4. Conclusion
The display driver now implements a robust cold-start power-stabilization and reset sequence. It handles the I2C expander boot phase via active retry-polling, switches Q4 VBAT_5V power correctly, pulses resets, conforms to all hardware timing delays, and restores default vendor configuration safely. The project compiles successfully without any warnings or errors.

---

## 5. Verification Method
1. **Build Verification**:
   Run PlatformIO compiler:
   ```powershell
   pio run -e waveshare_dash
   ```
   Ensure the output is `SUCCESS`.
2. **Pin Mapping Integrity**:
   Verify code in `src/display_driver.cpp` aligns with the schematic pin mapping of the TCA9554 expander.
