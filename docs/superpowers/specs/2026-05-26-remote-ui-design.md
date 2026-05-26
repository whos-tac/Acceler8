# Remote UI Redesign Specification (170x320 Vertical HUD)

## Goal
Redesign the LilyGo T-Display S3 remote controller screen to feature a highly polished, brutalist-cyberpunk vertical HUD UI. It aligns 100% with the high-contrast aesthetic of the main vehicle Dashboard, utilizing monospaced styling and pure ASCII symbols (no emojis) for high visibility under direct sunlight.

## Screen Configuration
* **Resolution:** 170px width by 320px height (Vertical Orientation).
* **LVGL Configuration:**
  - Width: `170`
  - Height: `320`
  - Rotation: `0` or configured for pure vertical orientation.

## Visual Theme & Palette
* **Background:** Solid black (`0x000000`).
* **Fonts:**
  - `lv_font_unscii_16` (or standard monospaced retro font) for labels, status, and power stats.
  - Medium-large block-style font for speed display inside the dial.
* **Colors:**
  - Purple (`0xC3B1E1`) for the Speed Dial header.
  - Green (`0x00FF88`) for the Board status and Board battery elements.
  - Cyan (`0x00CCCC`) for the Remote status and Remote battery elements.
  - Dim Gray (`0x555555`) for labels and inactive borders.
  - White (`0xFFFFFF`) for active readouts.

## Layout & Components

```
┌─────────────────────────────────┐
│ SIG: [#][#][#][-] | CAN: OK     │  <- Top ASCII Connection Status
├─────────────────────────────────┤
│                                 │
│             / 45 \              │  <- Circular/Arc Speed Dial
│            ( KM/H )             │
│             \____/              │
│                                 │
├─────────────────────────────────┤
│    ■■■■■■■■■■     ■■■■■■■■■■    │
│    ■ BOARD  ■     ■ REMOTE ■    │  <- Symmetrical Wide Battery Bars (32px)
│    ■■■■■■■■■■     ■■■■■■■■■■    │
│      42.0V          3.85V       │  <- Live Board/Remote Numeric Volts
├─────────────────────────────────┤
│ POWER: 380W                     │  <- Bottom Power Readout
└─────────────────────────────────┘
```

### 1. Header (Connection Status)
* **Text format:** `SIG: <ASCII_bars> | CAN: <status_text>`
  * Signal Strength: Maps ESP-NOW link RSSI to a 4-bar ASCII display: `[#][#][#][#]`, `[#][#][#][-]`, etc.
  * CAN Link Status: Displays `OK` (green) if dashboard telemetry is alive, or `!!` (accent red/orange) if disconnected.
* **Border:** Thin bottom line separates header from the dial.

### 2. Symmetrical Speed Dial (Center)
* **Visual:** A circular or semi-circular arc representing target or current speed (or power).
* **Speed Readout:** Large, bold center text displaying speed in km/h.
* **Sub-label:** Sits directly underneath the speed readout showing `"KM/H"` in a small font.

### 3. Wide Twin Battery Status (Middle-Bottom)
* **Board Battery (BRD):**
  * **Bar:** Vertical battery bar (`width: 32px`, `height: 50px`) with high contrast border. Fill height represents battery charge percentage (0-100%). Fill color is Green (`0x00FF88`).
  * **Numeric Volts:** Displays the absolute pack voltage underneath (e.g. `42.0V`).
* **Remote Battery (REM):**
  * **Bar:** Symmetrical vertical battery bar (`width: 32px`, `height: 50px`) with high contrast border. Fill color is Cyan (`0x00CCCC`).
  * **Numeric Volts:** Displays the cell voltage underneath (e.g. `3.85V`, representing typical Lipoly cell charge from 3.7V to 4.2V).

### 4. Footer (Power Readout)
* **Text Format:** `POWER: <watts>W`
* **Color:** Cyan (`0x00CCCC`) or green on regenerative braking.
* **Separation:** Sits nicely at the very bottom, separated by a thin top border line.

---

## Technical Details

### 1. Remote Battery Reading (New Feature)
* Read the LilyGo's internal battery sensing ADC (typically PIN 4 or specific battery ADC pin on LilyGo T-Display S3) inside the `RemoteApp::update()` loop.
* Convert the ADC raw reading to a voltage in the range `3.7V` to `4.2V`.
* Update the local `REM` display label and bar accordingly.

### 2. Board Telemetry Packet Update
* The receiver forwards the `TelemetryPacket` from the dashboard via ESP-NOW to the remote.
* The remote extracts the pack voltage (`battery_voltage_v`), power (`power_w`), speed (`speed_kmh`), and active CAN state (`can_alive`) to update the screen values dynamically in `remote_onDataRecv`.

---

## Verification Plan

### Manual / Simulation Verification
1. Run the native PlatformIO multi-window simulation.
2. Verify that the Remote window renders as a vertical `170x320` window.
3. Verify all graphics (dial, twin 32px battery bars, ASCII bars, and volts text) render flawlessly without overlapping or layout bugs.
4. Interact with the Dashboard/Receiver sliders in the simulation to confirm that values (Speed, Voltages, Power) update dynamically on the Remote's vertical screen.
