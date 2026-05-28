# UI Design System: ACCELER8 Brutalist Dashboard

## Aesthetic Goals
- **Brutalist Minimalism**: Heavy borders, zero radius, high contrast (Purple/Cyan on Black).
- **Glanceability**: Critical metrics (Speed, Power) must be readable at high vibration and long distance.
- **Pixel-Art Typography**: Exclusive use of bitmapped/retro fonts to avoid "soft" modern looks.

## Typography (VT323 / UNSCII)
| Font Identifier | Source | Purpose |
|---|---|---|
| `lv_font_block_56` | VT323 (56px) | **Speed** (Floating) and **Power** value. |
| `lv_font_block_24` | VT323 (24px) | Sub-metrics (Trip, Range, Wh/km, Temp values). |
| `lv_font_unscii_16` | UNSCII (16px) | Descriptive titles and unit labels. |

## Layout Architecture (480x480)
- **Margins**: 10px standard edge padding.
- **Hero Row (Y=25)**: 
    - **Speed**: Floating label on the left (no panel).
    - **Power**: Large square panel on the right (contains "WATT" title and value).
- **Gauge Row (Y=180)**:
    - Dedicated to **Motor Temp** and **ESC Temp**.
    - Layout: Full-width horizontal bars for maximum visual resolution of temperature gradients.
- **Stats Row (Y=350)**:
    - Three equal-width boxes: **TRIP**, **RANGE**, **Wh/km**.
- **Battery Strip (Y=430)**:
    - 10-segment discrete bar with a color gradient (Red → yellow → Green → Cyan).

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
