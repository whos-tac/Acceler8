# UI Design System: ACCELER8 Brutalist Dashboard

## Aesthetic Goals
- **Brutalist Minimalism**: Heavy borders, zero radius, high contrast (Purple/Cyan on Black).
- **Glanceability**: Critical metrics (Speed, Power) must be readable at high vibration and long distance.
- **Pixel-Art Typography**: Exclusive use of bitmapped/retro fonts to avoid "soft" modern looks.

## Interaction & Inputs
- **Remote D-Pad**: The primary method of input during riding. Holding **UP + DOWN** for 2 seconds opens the Settings Menu. **CONFIRM** (bit 4) is mapped to the horn during normal operation, and acts as "Enter" inside menus.
- **Touch Screen (GT911)**: The 480x480 screen features capacitive touch via GT911 on I2C. To prevent accidental touches while riding, touch events are strictly disabled in the UI until the "Touch Settings" modal is explicitly opened via the D-Pad.

## Typography (VT323 / UNSCII)
| Font Identifier | Source | Purpose |
|---|---|---|
| `lv_font_block_300` | VT323 (300px) | **Speed** (Huge floating label on left). |
| `lv_font_block_72` | VT323 (72px) | **Power** value inside the WATT box. |
| `lv_font_block_56` | VT323 (56px) | Sub-metrics values (Trip, Range, Wh/km, Temp) and Alerts. |
| `lv_font_block_24` | VT323 (24px) | Smaller sub-metrics (legacy use). |
| `lv_font_unscii_16` | UNSCII (16px) | Descriptive titles, unit labels, footer, and settings menus. |

## Layout Architecture (480x480)
- **Margins**: 8px standard edge padding.
- **Left Column (Primary Metrics)**:
    - **Speed**: Floating label using huge 300px font, filling the top left.
    - **Power**: Square panel below speed containing the "WATT" title and value.
- **Right Column (Secondary Stats)**:
    - Four equally-tall boxes stacked vertically: **TRIP**, **RANGE**, **Wh/km**, **ESC Temp**.
- **Battery Strip**:
    - 10-segment discrete bar above the footer with a color gradient (Red → Orange → Yellow → Green → Cyan).
- **Footer**:
    - "DASH v2.x" version label on left, CAN status (OK/ERR) on right.
- **Alert Overlay**:
    - Full-screen opaque red overlay with white text for critical warnings (CAN Timeout, Overtemp, Remote Disconnect).

## Color Palette
| Hex | Identifier | Use Case |
|---|---|---|
| `#C3B1E1` | `color_purple` | Speed, Motor telemetry. |
| `#00CCCC` | `color_cyan` | Power (Watts), ESC telemetry. |
| `#00FF88` | `color_green` | Regen state, CAN health OK. |
| `#FF3300` | `color_accent` | Overtemp alerts, Critical battery. |
| `#555555` | `color_dim` | Borders and background elements. |

## Key Logic Decisions
1. **Floating Speed**: The Speed value is not contained in a border box to allow it to be as large as possible without clipping concerns.
2. **Horizontal Temps**: Switched from vertical to horizontal bars to utilize the full width of the screen, making small temperature changes more obvious.
3. **No "KM/H" Tag**: The unit for speed is omitted as it is the most prominent element and contextually obvious to the rider.
