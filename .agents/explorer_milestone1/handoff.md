# Milestone 1 Explorer Handoff Report

## 1. Observation
Direct observations of source files from `c:\Users\thatw\Documents\Apollo-8\DashBoard\`:

### A. ESC & Battery Model (`src/simulation/esc_model.cpp`)
- **Throttle deadzone & safe start (lines 45-47)**:
  ```cpp
  if (!signal_lost && std::abs(input_throttle) <= 10.0f /* THROTTLE_DEADZONE */) {
      safe_start = true;
  }
  ```
- **Target throttle scaling (lines 53-62)**:
  ```cpp
  if (std::abs(input_throttle) <= 10.0f) {
      target = 0.0f;
  } else {
      float sign = (input_throttle > 0.0f) ? 1.0f : -1.0f;
      target = sign * (std::abs(input_throttle) - 10.0f) * (100.0f / (100.0f - 10.0f));
  }
  if (target < 0.0f) {
      target *= (50.0f / 50.0f);
  }
  ```
- **Ramping rates (lines 69-80)**:
  ```cpp
  if (signal_lost || settings_active || !safe_start) {
      current_rate = 200.0f; // FAILSAFE_COAST_RATE
  } else {
      bool accelerating = false;
      if (target > 0.0f && target > ramped_throttle && ramped_throttle >= 0.0f) {
          accelerating = true;
      } else if (target < 0.0f && target < ramped_throttle && ramped_throttle <= 0.0f) {
          accelerating = true;
      }
      current_rate = accelerating ? 75.0f /* RAMP_RATE_PER_SEC */ : 500.0f /* RAMP_DOWN_RATE_PER_SEC */;
  }
  ```
- **Battery Capacity and OCV (lines 114-129)**:
  ```cpp
  float base_drain = state.battery_current;
  float drain_multiplier = 1.0f;
  if (rapid_drain) {
      base_drain += 15.0f; // Drains 15A at idle
      drain_multiplier = 30.0f; // Scale up for visual feedback/test speed
  }
  float total_drain = base_drain * drain_multiplier;
  state.capacity_ah -= (total_drain * dt) / 3600.0f;
  ...
  float max_capacity = 10.0f;
  float ocv = 30.0f + 12.0f * (state.capacity_ah / max_capacity);
  ```
- **Currents & Voltage Sag (lines 131-144)**:
  ```cpp
  float duty_cycle = std::abs(state.ramped_throttle) / 100.0f;
  if (state.lvc_active) {
      state.motor_current = 0.0f;
      state.battery_current = 0.0f;
  } else {
      state.motor_current = duty_cycle * 50.0f; // Max 50A
      state.battery_current = state.motor_current * duty_cycle;
  }
  state.voltage_sag = state.battery_current * 0.1f;
  state.battery_voltage = ocv - state.voltage_sag;
  ```
- **LVC State Machine (lines 147-158)**:
  ```cpp
  if (state.battery_voltage < 32.0f) {
      state.lvc_active = true;
      state.motor_current = 0.0f;
      state.battery_current = 0.0f;
      state.voltage_sag = 0.0f;
      state.battery_voltage = ocv; // recovers to OCV when load is removed
  }
  if (state.lvc_active && state.capacity_ah >= 9.9f) {
      state.lvc_active = false;
  }
  ```
- **ERPM Calculation and Filter (lines 163-169)**:
  ```cpp
  float target_erpm = 0.0f;
  if (!state.lvc_active) {
      target_erpm = (state.ramped_throttle / 100.0f) * 80000.0f;
  }
  state.erpm = target_erpm - (target_erpm - state.erpm) * std::exp(-2.0f * dt);
  ```

### B. Receiver App (`src/receiver/receiver_app.cpp`)
- **Failsafe and Update Constants (lines 18-25)**:
  ```cpp
  #define MAX_DRIVE_CURRENT_A   50.0f
  #define MAX_BRAKE_CURRENT_A   50.0f
  #define THROTTLE_DEADZONE     10.0f
  #define RAMP_RATE_PER_SEC     75.0f
  #define RAMP_DOWN_RATE_PER_SEC 500.0f
  #define FAILSAFE_COAST_RATE   200.0f
  #define FAILSAFE_TIMEOUT_MS   250
  ```
- **D-Pad Event Handling & UI Buttons (lines 61-100, 144-160)**:
  ```cpp
  static void btn_up_event_handler(lv_event_t * e) {
      ... if (code == LV_EVENT_PRESSED) { sim_remote_btn_state |= (1 << 0); }
      else if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) { sim_remote_btn_state &= ~(1 << 0); }
  }
  // Down: bit 1, Left: bit 2, Right: bit 3, Confirm/OK: bit 4
  ...
  make_dpad_btn("UP", 127, 200, btn_up_event_handler);
  make_dpad_btn("LEFT", 57, 240, btn_left_event_handler);
  make_dpad_btn("OK", 127, 240, btn_confirm_event_handler);
  make_dpad_btn("RIGHT", 197, 240, btn_right_event_handler);
  make_dpad_btn("DOWN", 127, 280, btn_down_event_handler);
  ```
- **ESC Telemetry Chart (lines 163-177, 359-368)**:
  ```cpp
  chart_esc = lv_chart_create(lv_scr_act());
  lv_obj_set_size(chart_esc, 300, 235);
  lv_obj_align(chart_esc, LV_ALIGN_TOP_LEFT, 10, 330);
  lv_chart_set_type(chart_esc, LV_CHART_TYPE_LINE);
  lv_chart_set_range(chart_esc, LV_CHART_AXIS_PRIMARY_Y, 0, 800);
  ...
  lv_chart_set_next_value(chart_esc, ser_motor_current, (int)(sim_state.motor_current * 10.0f));
  lv_chart_set_next_value(chart_esc, ser_erpm, (int)(sim_state.erpm / 100.0f));
  lv_chart_set_next_value(chart_esc, ser_duty, (int)(std::abs(sim_state.ramped_throttle) * 8.0f));
  ```

### C. Remote App (`src/remote/remote_app.cpp`)
- **Remote Display Resolution & Layout (lines 265-272)**:
  ```cpp
  disp_drv.hor_res = 170;
  disp_drv.ver_res = 320;
  ...
  lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x000000), 0);
  ```
- **Status Header (`lbl_status`) (lines 275-280, 490-505)**:
  - Aligned mid-top at `(0, 6)`. Font `lv_font_unscii_8`. Color green `0x00FF88` (normal) or red `0xFF3300` (error).
- **Speed & Throttle Arc Gauges (lines 281-317, 604-611)**:
  ```cpp
  arc_speed = lv_arc_create(lv_scr_act());
  lv_obj_set_size(arc_speed, 110, 110);
  lv_arc_set_rotation(arc_speed, 135);
  lv_arc_set_bg_angles(arc_speed, 0, 270);
  lv_obj_align(arc_speed, LV_ALIGN_TOP_MID, 0, 22);
  lv_obj_set_style_arc_color(arc_speed, lv_color_hex(0x222222), LV_PART_MAIN);
  lv_obj_set_style_arc_color(arc_speed, lv_color_hex(0xC3B1E1), LV_PART_INDICATOR); // purple
  ...
  arc_throttle = lv_arc_create(lv_scr_act());
  lv_obj_set_size(arc_throttle, 110, 110);
  lv_arc_set_rotation(arc_throttle, 135);
  lv_arc_set_bg_angles(arc_throttle, 0, 270);
  lv_obj_align(arc_throttle, LV_ALIGN_TOP_MID, 0, 22);
  lv_obj_set_style_arc_opa(arc_throttle, LV_OPA_0, LV_PART_MAIN); // transparent
  lv_obj_set_style_arc_color(arc_throttle, lv_color_hex(0xFF9900), LV_PART_INDICATOR); // orange
  lv_obj_set_style_arc_opa(arc_throttle, LV_OPA_60, LV_PART_INDICATOR);
  lv_obj_set_style_arc_width(arc_throttle, 12, LV_PART_INDICATOR); // thicker
  lv_obj_move_foreground(arc_throttle);
  ```
- **Symmetrical Battery Columns (lines 318-384, 465-473, 554-563)**:
  - Board Battery (Left): Box size `32x50`, aligned at `(24, 150)`. Text labels `BRD` and volts (e.g. `42.0V`) below it. Fill height `bar_h = (int)(pct * 0.46f)` where `pct = ((v - 32.0f) / 10.0f) * 100.0f`.
  - Remote Battery (Right): Box size `32x50`, aligned at `(-24, 150)`. Text labels `REM` and volts (e.g. `3.92V`) below it. Fill height `bar_h = (int)(pct * 0.46f)` where `pct = ((rem_volts - 3.7f) / 0.5f) * 100.0f`.
- **Power Footer (lines 385-391, 476-488)**:
  - Aligned bottom-mid at `(0, -10)`. Displays `POWER: [W]W`. Text color is cyan `0x00CCCC` (drawing power) or green `0x00FF88` (regenerating).

### D. Dash App & Mechanical Config (`include/mechanical_config.h` & `src/ui_controller.cpp`)
- **Mechanical Constants (`src/mechanical_config.cpp` lines 4-6)**:
  ```cpp
  int motor_pole_pairs = 7;
  float gear_ratio = 4.0f;
  float wheel_diameter_mm = 200.0f;
  ```
- **Speed Math Formula (`include/mechanical_config.h` lines 22-28)**:
  ```cpp
  float rpm = (float)erpm / motor_pole_pairs;
  float wheel_rpm = rpm / gear_ratio;
  float circumference_m = (wheel_diameter_mm * PI) / 1000.0f;
  return (wheel_rpm * circumference_m * 60.0f) / 1000.0f;
  ```
- **Dashboard UI Layout (480x480) (`src/ui_controller.cpp` lines 69-102)**:
  - Left column: Width 300. Speed value (using 300px `lv_font_block_300` font) aligned to bottom-left of speed container. Watt box (90px height) at bottom.
  - Right column: Width 160, X = 312. Contains 4 boxes (`TRIP`, `RANGE`, `Wh/km`, `ESC Temp`) of height ~87px with 8px gaps.
  - Battery Strip: 10 horizontal bars above footer (height 44, Y = 412, width 40).
  - Footer: Y = 460. "DASH v2.2" on left, "CAN OK" on right.

---

## 2. Logic Chain
1. **Ramped Throttle**: Input potentiometer maps to a throttle range of $[-100.0, 100.0]\%$ with a $\pm 100$ count analog deadband ($\pm 10\%$ throttle deadzone). The target throttle scales linearly across the remaining range. Ramping applies frame time `dt`: accelerating forwards/backwards ramps at $75\%/s$; decelerating or braking decays at $500\%/s$; signal loss decays to $0\%$ at $200\%/s$ (`FAILSAFE_COAST_RATE`).
2. **Motor ERPM**: Calculated linearly from ramped throttle fraction ($[0.0, 1.0]$) up to a maximum of $80000$ ERPM. The value is smoothed using a first-order lag filter equation: $ERPM_k = Target + (ERPM_{k-1} - Target) \cdot e^{-2 \cdot dt}$, guaranteeing mathematical stability for any timestep `dt`.
3. **Currents & Sag**: Motor current is computed as $DutyCycle \cdot 50.0$ A (clamped to 0A under LVC). Battery current is $MotorCurrent \cdot DutyCycle$, which equals $DutyCycle^2 \cdot 50.0$ A. Voltage sag is computed by Ohm's Law across an internal resistance $R_{int} = 0.1\ \Omega$: $\Delta V = I_{bat} \cdot 0.1$. Real-time loaded voltage is $V_{bat} = OCV - \Delta V$.
4. **Capacity & Drain**: Drain rate (in Ah) reduces capacity via $Capacity_{k} = Capacity_{k-1} - \frac{I_{bat} \cdot Multiplier \cdot dt}{3600.0}$. The rapid drain multiplier scales total drain current by adding $15$ A and multiplying the result by $30.0$. Open Circuit Voltage (OCV) decays linearly with capacity from $42.0$ V at $10.0$ Ah down to $30.0$ V at $0.0$ Ah: $OCV = 30.0 + 12.0 \cdot \frac{Capacity}{10.0}$.
5. **LVC & Latching Bug**:
   - *Bug Mechanism*: In original code, LVC triggered at $V_{bat} < 32.0$ V, setting load current to 0A. This immediately removed the voltage sag, causing the battery voltage to jump back to OCV. If OCV was $\ge 33.0$ V (the original recovery check), LVC reset in the next frame. This generated a high-frequency LVC oscillation (chatter).
   - *Fix*: The state machine now latches `lvc_active = true`. Once active, it is ONLY cleared when a capacity recharge event occurs (i.e. capacity $\ge 9.9$ Ah), ignoring unloaded voltage bounce.
6. **D-Pad Event Handling**: The 5 buttons map to a bitmask state variable `sim_remote_btn_state` where pressed events set bits and released/lost events clear them: UP (bit 0), DOWN (bit 1), LEFT (bit 2), RIGHT (bit 3), OK/Confirm (bit 4).
7. **Telemetry Chart**: Plotted as a line chart with a Y-axis range from 0 to 800. Values are updated per-frame: Motor Current is scaled by $10.0$ (representing 0 to 80A), ERPM is divided by $100$ (representing 0 to 80,000 ERPM), and throttle magnitude is scaled by $8.0$ (representing 0 to 100% duty cycle).
8. **Visual Layouts**: Visual layout parameters (dimensions, coordinates, colors) are directly retrieved from `ui_controller.cpp` and `remote_app.cpp`, ensuring accurate alignment of UI assets and widgets.

---

## 3. Caveats
- *ponytail: config variability*: The speed calculation formula depends on `motor_pole_pairs`, `gear_ratio`, and `wheel_diameter_mm`. The values reported are default compile-time fallbacks. If the user edits settings via the UI, these variables are updated dynamically in RAM and stored in non-volatile storage (Preferences).

---

## 4. Conclusion

### A. Physics Equations & Constants

| Parameter / Physics Relation | Formula / Value | Constants |
| :--- | :--- | :--- |
| **Throttle Deadzone** | $\le 10\%$ of input range | `THROTTLE_DEADZONE = 10.0f` |
| **Throttle Acceleration Ramping** | $+75\%/\text{sec}$ | `RAMP_RATE_PER_SEC = 75.0f` |
| **Throttle Deceleration Decay** | $-500\%/\text{sec}$ | `RAMP_DOWN_RATE_PER_SEC = 500.0f` |
| **Failsafe Coasting Decay** | $-200\%/\text{sec}$ | `FAILSAFE_COAST_RATE = 200.0f` |
| **Failsafe Timeout** | $250\text{ ms}$ packet loss threshold | `FAILSAFE_TIMEOUT_MS = 250` |
| **Max Motor / Brake Current** | $50.0\text{ A}$ / $50.0\text{ A}$ | `MAX_DRIVE_CURRENT_A = 50.0f` |
| **Motor Current ($I_{motor}$)** | $DutyCycle \cdot 50.0\text{ A}$ | Max Current = $50.0\text{ A}$ |
| **Battery Current ($I_{bat}$)** | $I_{motor} \cdot DutyCycle = DutyCycle^2 \cdot 50.0\text{ A}$ | - |
| **Battery Internal Resistance** | $R_{int} = 0.1\ \Omega$ | - |
| **Voltage Sag ($\Delta V$)** | $I_{bat} \cdot 0.1\text{ V}$ ($0.0\text{ V}$ under active LVC) | $R_{int} = 0.1\ \Omega$ |
| **Open Circuit Voltage ($OCV$)** | $30.0 + 12.0 \cdot \frac{\text{Capacity Ah}}{10.0}$ V | Clamped $[30.0, 42.0]$ V |
| **Capacity Drain Rate** | $\Delta Ah = \frac{I_{bat, total} \cdot dt}{3600.0}$ (Max capacity = $10.0$ Ah) | - |
| **Rapid Drain Current** | $I_{bat, total} = (I_{bat} + 15.0) \cdot 30.0$ | Adds 15A idle, scales 30x |
| **Motor ERPM (Target)** | $DutyCycle \cdot 80000.0$ | Max ERPM = $80000.0$ |
| **Motor ERPM (Filtered)** | $ERPM_k = Target + (ERPM_{k-1} - Target) \cdot e^{-2 \cdot dt}$ | Lag coefficient $\tau = 0.5\text{ s}$ |
| **LVC Trigger Threshold** | $V_{bat} < 32.0\text{ V}$ | Sets `lvc_active = true` |
| **LVC Recovery Latch** | $\text{Capacity Ah} \ge 9.9\text{ Ah}$ | Prevents chatter on OCV recovery |

### B. Speed Math Parameters
- **Formula**:
  $$RPM_{motor} = \frac{ERPM}{PolePairs}$$
  $$RPM_{wheel} = \frac{RPM_{motor}}{GearRatio}$$
  $$\text{Circumference (m)} = \frac{\text{WheelDiameter (mm)} \cdot \pi}{1000}$$
  $$\text{Speed (km/h)} = \frac{RPM_{wheel} \cdot \text{Circumference (m)} \cdot 60}{1000}$$
- **Default Constants**:
  - `motor_pole_pairs = 7`
  - `gear_ratio = 4.0`
  - `wheel_diameter_mm = 200.0`
  - *ponytail: combined speed factor*: $\text{Speed (km/h)} \approx ERPM \cdot 0.001346397$ (at default config).

### C. Display & Layout Specifications

#### 1. Remote Screen (170x320, vertical, black background `0x000000`)
- **Status Header (`lbl_status`)**: Aligned mid-top `(0, 6)`. Font `lv_font_unscii_8`. Color `0x00FF88` (green) or `0xFF3300` (red on connection error).
- **Speed Dial (`arc_speed`)**: Centered at `(0, 22)`, diameter 110px. Active color purple `0xC3B1E1`, background color `0x222222`. Angle range 0 to 270 (rotated 135 deg). Knob hidden.
- **Throttle Overlay (`arc_throttle`)**: Co-located with speed dial. Active color orange `0xFF9900` at 60% opacity. 0% opacity background. Width 12px (thicker). Foreground z-ordered.
- **Speed Text (`lbl_speed`)**: Centered inside dial at `(0, 54)`. Font `lv_font_montserrat_36` (white).
- **Unit Text (`lbl_speed_unit`)**: Centered at `(0, 92)`. Font `lv_font_unscii_16` (gray `0x555555`). Text "KM/H".
- **Board Battery (Left)**: Background box `32x50` at `(24, 150)` (color `0x111111` / border `0x333333`). Active fill color green `0x00FF88`, height scaled linearly between $32.0$ V ($0$px) and $42.0$ V ($46$px). Labels `BRD` and voltage below.
- **Remote Battery (Right)**: Background box `32x50` at `(-24, 150)` (color `0x111111` / border `0x333333`). Active fill color cyan `0x00CCCC`, height scaled linearly between $3.7$ V ($0$px) and $4.2$ V ($46$px). Labels `REM` and voltage below.
- **Power Footer (`lbl_power`)**: Aligned bottom-mid at `(0, -10)`. Displays `POWER: [W]W`. Font `lv_font_unscii_16`. Color: cyan `0x00CCCC` (positive power) or green `0x00FF88` (negative/regen power).

#### 2. Dashboard Screen (480x480, black background `0x000000`)
- **Speed Container**: Size `300x302` at `(8, 8)`. Contains `label_speed_val` aligned bottom-left using `lv_font_block_300` font (purple `0xC3B1E1`).
- **Watt Hero Box**: Size `300x90` at `(8, 314)`. Active color cyan `0x00CCCC`, font `lv_font_block_72`. Switches to green `0x00FF88` when power < 0 (regen).
- **Right Stats Column**: Width 160, X = 312. 4 stacked boxes of height ~87px with 8px gaps:
  - Box 1: `TRIP` (Y = 8, value font block_56, color white `0xFFFFFF`).
  - Box 2: `RANGE` (Y = 103, value font block_56, color cyan `0x00CCCC`).
  - Box 3: `Wh/km` (Y = 198, value font block_56, color green `0x00FF88`).
  - Box 4: `ESC Temp` (Y = 293, value font block_56, color purple `0xC3B1E1`, shifts to red `0xFF3300` when > 70C).
- **Battery Strip**: 10 horizontal bars at `Y = 412`, height 44, segment width 40. Active bars are lit up in gradient colors from left (red) to right (cyan) based on battery voltage fraction.
- **Footer**: `Y = 460`. "DASH v2.2" at left (dim `0x555555`), "CAN OK" at right (green `0x00FF88` or red `0xFF3300` on error). Font `lv_font_unscii_16`.

#### 3. Receiver App UI (Control Panel)
- **Status Text Label**: Top-left at `(10, 10)`. Displays LVC state, signal state, inputs, speed, current, voltage, sag, power, capacity.
- **Comm Loss / Rapid Drain Checkboxes**: Aligned at `(10, 110)` and `(170, 110)`.
- **Recharge Button**: Size `300x35` at `(10, 150)`. Resets capacity to 10.0 Ah and clears LVC state.
- **D-Pad simulation buttons**: Size `65x32`. UP `(127, 200)`, LEFT `(57, 240)`, OK `(127, 240)`, RIGHT `(197, 240)`, DOWN `(127, 280)`.
- **ESC Telemetry Chart**: Size `300x235` at `(10, 330)`. Line chart type. Y-axis range `[0, 800]`.
  - Red series (`ser_motor_current`): Motor current scaled by 10.0.
  - Green series (`ser_erpm`): ERPM divided by 100.0.
  - Blue series (`ser_duty`): Absolute throttle percentage scaled by 8.0.

---

## 5. Verification Method

### A. Test Execution
The physics equations, throttle ramping behavior, voltage sag, low-voltage cutoff, LVC latching logic, and stability under large timesteps can be verified by running the native test suite.

Run the following commands in the workspace root:
```powershell
# 1. Compile the native test program
pio run -e native_tests

# 2. Run the compiled executable
.pio/build/native_tests/program.exe
```

Expected output:
```text
Running test_throttle_mapping...
test_throttle_mapping PASSED
Running test_ramping_failsafe...
test_ramping_failsafe PASSED
Running test_battery_model...
test_battery_model PASSED
Running test_lvc_chatter_fix...
test_lvc_chatter_fix PASSED
Running test_integration_stability...
test_integration_stability PASSED
Running test_brake_throttle_scaling...
test_brake_throttle_scaling PASSED
Running test_nan_sanitization...
test_nan_sanitization PASSED
ALL TESTS PASSED SUCCESSFULLY!
```

### B. Invalidation Conditions
The extracted physics parameters or equations will be invalidated if:
1. `src/simulation/esc_model.cpp` or `include/mechanical_config.h` are modified (altering ramping rates, LVC thresholds, or speed calculations).
2. The UI files (`src/ui_controller.cpp` or `src/remote/remote_app.cpp`) are edited, changing layout alignment or widget scaling factors.
3. The platformio configuration for `native_tests` changes its sources or include paths.
