# AI Workflow: UI Development (`ui_controller.cpp`)

This document describes how to correctly modify the LVGL user interface for the ACCELER8 dashboard.

---

## Ground Rules

1. **All UI logic lives in `src/ui_controller.cpp`**.
2. **Always compile and run the native simulator after every change.**
3. **Do not use `lv_obj_set_style_transform_zoom`.** It breaks the pixel-art bitmapped text.
4. **Current Layout Logic**:
   - **Hero Row**: Floating Speed label (Left), border-boxed Power (Right).
   - **Gauge Row**: High-resolution wide horizontal bars for Motor/ESC temps.
   - **No Tagging**: Speed is unit-less (KM/H implied). Power is labeled "WATT".
5. **Removed Tiles**: Do not add Duty Cycle, Motor Amps, or Battery Amps tiles back without explicit instruction. They were removed to prioritize readability.

---

## Fonts Available

| Font Identifier | Size | Character Set | Use For |
|---|---|---|---|
| `lv_font_block_56` | 56px | Full ASCII | Hero numbers (speed, wattage) |
| `lv_font_block_24` | 24px | Full ASCII | Bar values, arc values, stat boxes |
| `lv_font_unscii_16` | 16px | ASCII | Tile titles, unit labels |
| `lv_font_unscii_8` | 8px | ASCII | Footer text |

> The `block_*` fonts are generated from **VT323** (a retro monospace terminal font from Google Fonts). Source TTF: `blocky.ttf` in the project root.  
> To regenerate at a new size: `cmd /c "npx -y lv_font_conv --bpp 1 --size XX --font blocky.ttf -o src/lv_font_block_XX.c --format lvgl -r 0x20-0x7F"`

---

## Layout Constants

All geometry is defined as `static const int` at the top of the `UIController` namespace in `ui_controller.cpp`. The screen is **480×480 pixels**.

```
MARGIN = 10px (screen edge padding)
HERO_Y, GAUGE_Y, STATS_Y, BAR_Y = zone Y offsets
```

Always compute positions from constants — never hardcode raw pixel values inside widget builders.

---

## Widget Builder Pattern

Every reusable widget is a function in the `UIController` namespace. The pattern is:

```cpp
lv_obj_t* create_X_widget(int x, int y, int w, int h, ..., lv_obj_t** val_label, lv_color_t color, ...) {
    // 1. Create base object / bar / arc on lv_scr_act()
    // 2. Set size + align to TOP_LEFT + x,y
    // 3. Apply style (bg color, border, radius=0 for brutalist look)
    // 4. Create title label → align TOP_LEFT
    // 5. Create value label → align BOTTOM_RIGHT, flush to edge (offset 0,0)
    // 6. Return the widget pointer for later dynamic updates in update()
}
```

**Key constraint**: Value labels must be anchored to `LV_ALIGN_BOTTOM_RIGHT` with `(0, 0)` offset so large numbers flush perfectly against the block edge.

---

## Color Palette

| Name | Hex | Semantic |
|---|---|---|
| `color_purple` | `#C3B1E1` | Speed, Motor Temp (primary accent) |
| `color_blue` | `#2F6A87` | Battery current (fjord blue) |
| `color_cyan` | `#00CCCC` | Duty, ESC Temp, Watt (secondary accent) |
| `color_amber` | `#FFAA00` | Warning / high load |
| `color_green` | `#00FF88` | Regen / CAN OK / healthy state |
| `color_accent` | `#FF3300` | Critical alert (overtemp, CAN dead) |
| `color_white` | `#FFFFFF` | Trip distance text |
| `color_dim` | `#555555` | Inactive borders, background grid |

---

## Updating Dynamic Values (`update()`)

The `update()` function is called every LVGL tick from `main.cpp`. It reads from `g_vehicle_state` (declared in `can_driver.h`) and pushes new values into widget labels/bars using:

- `lv_label_set_text(label, buf)` — for text labels
- `lv_bar_set_value(bar, value, LV_ANIM_OFF)` — for progress bars
- `lv_arc_set_value(arc, value)` — for arc gauges
- `lv_obj_set_style_bg_color(widget, color, LV_PART_INDICATOR)` — for dynamic color changes (e.g., regen state)

**Always use `snprintf(buf, sizeof(buf), ...)` before calling `lv_label_set_text`.** Never pass a raw float directly.

---

## What Not to Do

- ❌ Do not create child `lv_obj_t` containers inside `lv_bar_create()` — the bar's `INDICATOR` part clips children.
- ❌ Do not call `lv_obj_add_flag(bar, LV_OBJ_FLAG_SCROLLABLE)` — bars are not scrollable.
- ❌ Do not use `LV_ALIGN_CENTER` for value labels in blocks with titles — they will overlap. Use `BOTTOM_RIGHT` / `BOTTOM_LEFT`.
- ❌ Do not add 3 selector arguments to simple LVGL style calls on `LV_PART_MAIN` — just pass `0` as the selector.
