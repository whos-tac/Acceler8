# Telemetry Formulas: Physics & Conversions

This document centralizes all mathematical conversions used in the Apollo-8 DashBoard.

## 1. Speed (KM/H)
Derived from Motor ERPM.

**Formula:**
$$Speed = \frac{ERPM}{PolePairs \times GearRatio} \times \frac{Diameter \times \pi \times 60}{1,000,000}$$

**Constants (from `mechanical_config.h`):**
*   `MOTOR_POLE_PAIRS`: 7
*   `GEAR_RATIO`: 4.0
*   `WHEEL_DIAMETER_MM`: 200.0

**Implementation (`mechanical_config.h`):**
```cpp
inline float calculate_speed_kmh(int32_t erpm) {
    float rpm = (float)erpm / MOTOR_POLE_PAIRS;
    float wheel_rpm = rpm / GEAR_RATIO;
    float circumference_m = (WHEEL_DIAMETER_MM * PI) / 1000.0f;
    return (wheel_rpm * circumference_m * 60.0f) / 1000.0f;
}
```

## 2. Power (Watts)
Instantaneous power draw from the battery pack.

**Formula:**
$$P = V_{avg} \times I_{total}$$

*   $V_{avg}$: Average voltage of all connected ESCs.
*   $I_{total}$: Sum of battery currents from all connected ESCs.

## 3. Energy Consumption (Watt-Hours)
Calculated by integrating Power over time.

**Formula:**
$$Wh_{session} = \sum (Power \times \Delta t)$$

*   $\Delta t$: Time since last update in hours.
*   **Logic**: Only positive power draw is integrated (regenerated energy is currently excluded from the Wh total to keep "energy consumed from battery" accurate).

## 4. Efficiency (Wh/km)
Average energy usage over the distance traveled.

**Formula:**
$$\text{Efficiency} = \frac{Wh}{Distance}$$

*   $Distance$: Total distance in KM (derived from tachometer counts).

## 5. Estimated Range (KM)
Calculated based on remaining battery capacity and real-time efficiency.

**Formula:**
$$Range = \frac{Wh_{remaining}}{\text{Efficiency}}$$

*   $Wh_{remaining}$: `BATTERY_TOTAL_WH` × `Battery %`.
*   **Safety Fallback**: If less than 0.5km has been traveled or efficiency is unknown, the system uses a conservative estimate of **25 Wh/km**.

## 6. Battery Percentage (%)
A linear approximation based on resting voltage.

**Formula (for 10S pack):**
$$Pct = \frac{V_{avg} - 32.0V}{10.0V} \times 100$$
*   Range is clamped between 0% (32V) and 100% (42V).
