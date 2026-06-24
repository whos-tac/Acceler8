## 2026-06-24T08:29:49Z

You are Worker 1, a versatile worker agent (archetype: teamwork_preview_worker).
Your working directory for metadata is: c:\Users\thatw\Documents\Apollo-8\DashBoard\.agents\worker_boot_reliability_1
The target codebase is at: c:\Users\thatw\Documents\Apollo-8\DashBoard

Mission:
Implement the boot rendering reliability fix in `src/display_driver.cpp` based on the recommendations from the Explorer agents.

Style:
Operating under /ponytail mode (Lazy senior dev mode: build the minimum that works, avoid unrequested abstractions, mark simplifications with a ponytail: comment). Ensure all changes strictly follow this style.

Reference Material:
Please refer to the following files in the Explorer 3 directory for detail:
- Handoff Report: c:\Users\thatw\Documents\Apollo-8\DashBoard\.agents\teamwork_preview_explorer_boot_reliability_3\handoff.md
- Proposed file: c:\Users\thatw\Documents\Apollo-8\DashBoard\.agents\teamwork_preview_explorer_boot_reliability_3\proposed_display_driver.cpp
- Patch file: c:\Users\thatw\Documents\Apollo-8\DashBoard\.agents\teamwork_preview_explorer_boot_reliability_3\display_driver_reset.patch

Tasks:
1. Initialize your BRIEFING.md and progress.md in your working directory. Set up a heartbeat cron.
2. Edit `src/display_driver.cpp` to implement the proper reset sequence and stabilization delays:
   - Add a 100ms startup delay at the beginning of `DisplayDriver::init()` to let power rails stabilize.
   - Configure TCA9554 IO Expander Direction Register (0x03) to `0x32` (P0 LCD_RST output, P2 LCD_BL output, P3 TP_RST output). Add a ponytail style comment explaining this change.
   - Pulse resets: Write `0xC0` to Output Register (0x01) to pull resets and backlight low, wait 20ms, then write `0xC9` to release reset pins (driving P0/P3 high, while keeping P2 LCD_BL low).
   - Wait 120ms for LCD internal POR/calibration before calling `gfx->begin()`.
   - After completing display and touch setup, write `0xFF` to Output Register (0x01) to turn on the backlight.
3. Verify that the project compiles successfully using:
   pio run -e waveshare_dash
4. Document the exact changes applied, build commands run, and build results in handoff.md in your working directory.
5. Send a message to the orchestrator (conversation ID: 01d0198a-8c86-402e-859c-a38b0b0b6997) notifying that you are done, with build verification evidence and your handoff.md path.

MANDATORY INTEGRITY WARNING:
DO NOT CHEAT. All implementations must be genuine. DO NOT hardcode test results, create dummy/facade implementations, or circumvent the intended task. A Forensic Auditor will independently verify your work. Integrity violations WILL be detected and your work WILL be rejected.

## 2026-06-24T08:30:42Z

**Context**: Boot rendering reliability fix on Waveshare Dashboard.
**Content**: Explorer 2 has completed schematic verification and identified a crucial correction to the TCA9554 pin mapping. Pin 0 controls VBAT_5V enable (active LOW, switches Q4 MOSFET to power 5V subsystems/backlight), Pin 1 is TP_RST (active LOW), Pin 2 is TP_INT (touch interrupt), and Pin 3 is LCD_RST (active LOW). 
Using Explorer 3's logic would write 0xC9 (driving Pin 0 HIGH), which would disable Q4 and cut the 5V power to the display.
Therefore, please use Explorer 2's reset sequence instead:
1. Probe address 0x24 on the I2C bus in a retry loop (up to 15 times, 10ms delay between retries) to wait for CH32V003 coprocessor to boot. Print a warning to Serial if it fails.
2. If expander is ready:
   - Drive all outputs LOW (0x00 write to Register 0x01) to enable VBAT_5V power, hold LCD_RST low, hold TP_RST low, and hold TP_INT low.
   - Set Pins 0, 1, 2, 3 as outputs by writing 0x30 to Configuration Register (0x03).
   - Wait 20ms delay.
   - Release resets by driving Pin 1 (TP_RST) and Pin 3 (LCD_RST) HIGH, while keeping Pin 0 (VBAT_5V EN) and Pin 2 (TP_INT) LOW. This corresponds to writing 0x0a to Output Register (0x01).
   - Wait 120ms delay for screen and touch controller POR.
   - Restore the original vendor pin direction configuration by writing 0x3a to Configuration Register (0x03).
**Action**: Update your implementation in src/display_driver.cpp to use the corrected sequence described above, verify compilation via `pio run -e waveshare_dash`, and report back in your handoff.
