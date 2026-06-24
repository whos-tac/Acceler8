# BRIEFING — 2026-06-24T10:26:17+02:00

## Mission
Investigate boot and UI initialization sequence for Waveshare Dashboard to resolve intermittent cold start blank screen issue.

## 🔒 My Identity
- Archetype: teamwork_preview_explorer
- Roles: read-only explorer
- Working directory: c:\Users\thatw\Documents\Apollo-8\DashBoard\.agents\teamwork_preview_explorer_boot_reliability_1
- Original parent: 01d0198a-8c86-402e-859c-a38b0b0b6997
- Milestone: Boot Reliability Investigation

## 🔒 Key Constraints
- Read-only investigation — do NOT implement
- Operating under /ponytail mode (Lazy senior dev style: build minimum that works, avoid unrequested abstractions, mark simplifications with ponytail comments)

## Current Parent
- Conversation ID: 01d0198a-8c86-402e-859c-a38b0b0b6997
- Updated: 2026-06-24T10:26:17+02:00

## Investigation State
- **Explored paths**:
  - `src/main.cpp`: Main setup and loop
  - `src/dash_app.cpp`: DashApp setup
  - `src/display_driver.cpp`: Display driver and I2C/SPI initialization
  - `hardware_pinout.md`: GPIO pin mappings
  - `.pio/libdeps/waveshare_dash/TAMC_GT911/`: GT911 library sources
- **Key findings**:
  - TCA9554 IO Expander controls GT911 touch reset (P0) and ST7701 display reset (P2).
  - The display initialization code in `src/display_driver.cpp` never writes to Register 1 (Output Port) of TCA9554 to assert/deassert reset. It only sets pin directions.
  - Lacking an explicit hardware reset toggle and sufficient delay (120ms) after reset release before starting display communications via `gfx->begin()` leads to the display occasionally ignoring command sequences, causing the intermittent blank screen on cold starts.
- **Unexplored areas**: None. The root cause has been isolated and verified.

## Key Decisions Made
- Isolated root cause to missing hardware reset sequence and boot delay for the ST7701 display and GT911 touch controller via TCA9554 IO expander.
- Proposed clean fix patching `src/display_driver.cpp` (in `/ponytail` mode).
- Verified codebase compiles successfully using `pio run -e waveshare_dash`.

## Artifact Index
- c:\Users\thatw\Documents\Apollo-8\DashBoard\.agents\teamwork_preview_explorer_boot_reliability_1\handoff.md — Investigation report
- c:\Users\thatw\Documents\Apollo-8\DashBoard\.agents\teamwork_preview_explorer_boot_reliability_1\boot_reset_fix.patch — Proposed code fix patch
