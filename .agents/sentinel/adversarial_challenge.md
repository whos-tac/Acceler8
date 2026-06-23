## Challenge Summary

**Overall risk assessment**: LOW

The overall risk of the simulation implementation is low due to robust bounds checking, loop iteration limits, and logical isolation of simulation code from production firmware targets.

## Challenges

### [Low] Challenge 1: LVC Rapid Oscillation (Chatter)
- **Assumption challenged**: The simulation assumes that a 1.0V hysteresis window (cutoff at 32.0V, recovery at 33.0V) is sufficient to prevent rapid oscillation of the Low-Voltage Cutoff.
- **Attack scenario**: If the battery capacity is partially depleted such that the Open-Circuit Voltage (OCV) is 33.1V:
  1. The voltage is above 33.0V, so LVC is inactive.
  2. The user applies 100% throttle, which draws 50A.
  3. This causes a 5.0V sag (50A * 0.1 Ohm), dropping the loaded voltage to 28.1V.
  4. LVC is immediately triggered (< 32.0V), shutting off the motor and current.
  5. The load is removed, so sag becomes 0V, and the voltage bounces back to OCV (33.1V).
  6. In the next frame, because the voltage is >= 33.0V, LVC is cleared, and the cycle repeats.
- **Blast radius**: Simulated speed and telemetry values will oscillate rapidly between normal and cutoff states, which may cause jumpy UI rendering and charts.
- **Mitigation**: While acceptable for a test simulation, a robust mitigation would be to require a "Recharge Battery" action or a higher recovery threshold (e.g., 34.0V) to exit LVC, or introduce a timer-based lock out (e.g., LVC must remain active for at least 5 seconds once triggered).

### [Low] Challenge 2: Floating point time step (dt) instability
- **Assumption challenged**: The time delta `dt` calculated from `millis()` is assumed to be small and well-behaved.
- **Attack scenario**: If the simulator process is suspended (e.g. by dragging the console window or pausing in a debugger), the next tick will calculate an extremely large `dt` (e.g., seconds or minutes).
- **Blast radius**: Large `dt` could cause the ERPM lag filter (`state.erpm += erpm_diff * dt * 2.0f`) to overshoot or produce `NaN` values, destabilizing the physics state.
- **Mitigation**: The code actively mitigates this in `receiver_app.cpp` by clamping `dt`:
  ```cpp
  if (dt <= 0.0f) dt = 0.001f;
  if (dt > 0.5f) dt = 0.1f;
  ```
  This is highly robust and prevents any overflow/NaN issues.

## Stress Test Results

- **Negative/Zero Time Step** → `dt` is clamped to `0.001f` → Simulation remains stable, no division by zero → **PASS**
- **Large Time Step (Debugger Pause)** → `dt` is clamped to `0.1f` → Simulation recovers smoothly without overshooting or numerical explosion → **PASS**
- **Boundary Condition Loop (Pass-through zero)** → `calculate_ramped_throttle` loop executes safely and clamps iterations at 10 → No CPU lockup or infinite loop → **PASS**

## Unchallenged Areas

- **PlatformIO / Toolchain configuration** — We assume that PlatformIO's native toolchain matches the target OS compiler settings. This is outside our code review scope.
