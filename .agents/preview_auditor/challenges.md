# Adversarial Review - Redesigned Digital Test Environment

## Challenge Summary

**Overall risk assessment**: LOW

## Challenges

### [Low] Challenge 1: Hardcoded MAC Address Routing Dependency
- **Assumption challenged**: Static mock MAC addresses will always remain synchronized between individual application components.
- **Attack scenario**: If a developer modifies a MAC address in `espnow_receiver.cpp` (e.g. `dash_mac`) but forgets to update `sim_main.cpp`'s static packet routing arrays, ESP-NOW communication will fail silently, halting the closed-loop telemetry flow.
- **Blast radius**: Breaking telemetry updates to the Dashboard or control inputs to the Receiver.
- **Mitigation**: Define the mock MAC addresses in a single shared header (e.g. `espnow_packets.h`) so any changes propagate automatically to both the applications and the simulation router.

### [Low] Challenge 2: Loop Latency & Spurious Failsafe Triggers
- **Assumption challenged**: The host environment running the simulator will always maintain low scheduling latency (<250ms).
- **Attack scenario**: Under heavy system load or when debugging/stepping through code, the simulator main loop may stall for more than 250ms. This will trigger a `signal_lost` condition in the Receiver app, resetting `safe_start` and causing the motor to coast down even if no real connection loss occurred.
- **Blast radius**: Transient motor cuts during heavy CPU usage.
- **Mitigation**: The current `FAILSAFE_TIMEOUT_MS` of 250ms is appropriate for real hardware safety, but in simulated environments, it could optionally be increased if execution is paused or debugged.

### [Low] Challenge 3: Rapid battery depletion state preservation
- **Assumption challenged**: The battery state remains within expected ranges under sustained rapid drain.
- **Attack scenario**: If `Rapid Drain` is left enabled, the battery capacity drains at 450A equivalent. The capacity quickly reaches `0.0 Ah` and remains there. The LVC triggers and cuts throttle to 0. Since the idle drain is still active under `rapid_drain` (15A * 30 = 450A), the capacity stays at 0.
- **Blast radius**: The system enters a permanent LVC shutdown state until the "Recharge Battery" button is manually clicked.
- **Mitigation**: The recharging button clears LVC and resets capacity to 10.0Ah. This works exactly as designed.

## Stress Test Results

- **Time-step spike (long pause)** → Simulator paused for 2.0s → `dt` capped to `0.1s` by physics integration boundary guards → Pass (No math/overflow instabilities).
- **Infinite Rapid Drain** → Battery runs down to 0Ah → LVC cuts current to 0A, status label shows "LVC [NO SIGNAL]" and motor/battery current are cut → Pass.
- **Recharge in Cutoff** → Click recharge button → Capacity reset to 10.0Ah, LVC active flag cleared, normal operation resumes → Pass.
- **LVC Latching and Chatter Prevention** → Run battery down until LVC triggers, then remove load to let OCV bounce back to 33.6V → LVC state machine remains latched until full recharge occurs → Pass (no chatter, latch works correctly).
- **Integration Stability under Extreme dt** → Update physics with dt = 72.0s → ERPM converges stably to target_erpm without diverging or overflowing → Pass (analytical lag filter guarantees stability).

## Unchallenged Areas

- **CAN hardware framing (Twai driver)** — Reason not challenged: Physical CAN hardware behavior is mocked out in the native environment (`#ifndef ARDUINO`), so only the ESP-NOW mock packet flow is tested in this audit.
