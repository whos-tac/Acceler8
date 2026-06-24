# Plan — Boot Reliability Fix

This plan details the steps to implement, review, and verify the boot reliability fix on the Waveshare Dashboard.

## Steps

### Step 1: Code Implementation
Apply the initialization sequence fixes to `src/display_driver.cpp` following the `/ponytail` style:
- Add a 100ms startup delay to allow the power rails to stabilize.
- Configure TCA9554 IO Expander Direction Register (0x03) to `0x32` (P0 LCD_RST output, P2 LCD_BL output, P3 TP_RST output).
- Write `0xC0` to Output Register (0x01) to pull LCD_RST, TP_RST, and LCD_BL low for 20ms.
- Write `0xC9` to Output Register (0x01) to release reset on LCD_RST and TP_RST (pull high) while keeping LCD_BL low.
- Wait 120ms for ST7701 internal stabilization before calling `gfx->begin()`.
- After display and touch controllers have been initialized, write `0xFF` to Output Register (0x01) to turn on the backlight.

### Step 2: Build Verification
Run the build command for waveshare_dash:
```powershell
pio run -e waveshare_dash
```
Verify that the compilation succeeds.

### Step 3: Peer Review
Spawn two independent Reviewers to review the code change for correctness, style, and interface conformance.

### Step 4: Challenger Verification
Spawn a Challenger to run stress tests/verifications to ensure the boot sequence is robust and compile verification passes.

### Step 5: Forensic Audit
Spawn a Forensic Auditor to ensure no cheating or dummy implementations were introduced.

### Step 6: Final Reporting & Closure
Synthesize reviews, check the gate criteria, update the PROJECT.md and progress.md files, and report to the user.
