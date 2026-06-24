# BRIEFING — 2026-06-24T10:31:00+02:00

## Mission
Investigate the boot and UI initialization sequence for the Waveshare Dashboard (PlatformIO environment waveshare_dash) to find the cause of the intermittent boot issue where the screen stays blank on cold starts.

## 🔒 My Identity
- Archetype: teamwork_preview_explorer
- Roles: Teamwork explorer
- Working directory: c:\Users\thatw\Documents\Apollo-8\DashBoard\.agents\teamwork_preview_explorer_boot_reliability_2
- Original parent: 01d0198a-8c86-402e-859c-a38b0b0b6997
- Milestone: Boot reliability investigation

## 🔒 Key Constraints
- Read-only investigation — do NOT implement
- Operating under /ponytail mode (minimum that works, avoid unrequested abstractions, mark simplifications with a ponytail: comment)
- Network mode: CODE_ONLY (no external websites/services)

## Current Parent
- Conversation ID: 01d0198a-8c86-402e-859c-a38b0b0b6997
- Updated: 2026-06-24T10:31:00+02:00

## Investigation State
- **Explored paths**: `src/main.cpp`, `src/display_driver.cpp`, `src/dash_app.cpp`, `platformio.ini`, `hardware_pinout.md`, `Resources/Waveshare Board Dash/demo_repo/Schematic/ESP32-S3-Touch-LCD-4 V4.0.pdf`
- **Key findings**:
  1. The board uses a CH32V003 coprocessor (at I2C address `0x24`) programmed to act as a TCA9554 IO expander. It requires a boot delay on cold start, but the original code talks to it immediately, causing I2C writes to fail silently.
  2. The display reset (`LCD_RST` on Pin 3) and touch reset (`TP_RST` on Pin 1) are configured as Inputs, meaning no hardware reset is performed.
  3. Corrected a previous agent's pinout mapping by extracting connections directly from the schematic PDF: Pin 0 = `VBAT_5V` enable, Pin 1 = `TP_RST`, Pin 2 = `TP_INT`, Pin 3 = `LCD_RST`, Pin 6 = `BEE_EN`.
  4. Proposed a robust I2C probe-and-retry sequence and a complete hardware reset sequence with proper stabilization delays (20ms reset low, 120ms post-reset sleep out delay).
- **Unexplored areas**: None (investigation complete)

## Key Decisions Made
- Confirmed exact TCA9554 / CH32V003 pin mappings from schematic.
- Formulated the exact reset register sequence.

## Artifact Index
- c:\Users\thatw\Documents\Apollo-8\DashBoard\.agents\teamwork_preview_explorer_boot_reliability_2\ORIGINAL_REQUEST.md — Original request description
- c:\Users\thatw\Documents\Apollo-8\DashBoard\.agents\teamwork_preview_explorer_boot_reliability_2\extract_schematic.py — Script used to search PDF schematic
- c:\Users\thatw\Documents\Apollo-8\DashBoard\.agents\teamwork_preview_explorer_boot_reliability_2\schematic_text.txt — Plaintext dump of the schematic PDF
