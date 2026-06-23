## 2026-06-23T08:32:01+02:00

Implement the ESC physics model in JavaScript and verify it using a Node.js unit test suite.

Working folder: `web_sim` (Please create it)

Please operate strictly in /ponytail mode: build the minimum that works, avoid unrequested abstractions, and mark any simplifications with a `// ponytail:` comment.

Specific tasks:
1. Read the explorer handoff report at `c:\Users\thatw\Documents\Apollo-8\DashBoard\.agents\explorer_milestone1\handoff.md` to retrieve the physics formulas, constants, and constraints.
2. Create `web_sim/esc_model.js`. Implement a self-contained ES6 class `ESCModel` (or simple state object/functions) matching the behavior of `esc_model.cpp`.
   - Ramped throttle with deadzone and safe-start.
   - ERPM calculations using the mathematically stable lag filter: `erpm = target - (target - erpm) * Math.exp(-2.0 * dt)`.
   - Motor current and battery current calculation (current sag, sag voltage, loaded voltage).
   - Battery capacity depletion and rapid drain.
   - Fixed LVC state machine: LVC triggers at battery voltage < 32V and sets `lvc_active = true`. Once `lvc_active` is true, motor current, battery current, and voltage sag are 0, and the loaded voltage is equal to OCV. LVC must remain latched until capacity is recharged to >= 9.9 Ah.
   - Support a recharge function that resets capacity to 10.0 Ah and clears the LVC latch.
3. Create `web_sim/test_physics.js`. Implement unit tests using simple Node assertions to verify:
   - Throttle ramping (accelerating at 75%/s, decelerating at 500%/s).
   - Failsafe triggers (throttle coasting to 0 at 200%/s on comm loss after 250ms timeout).
   - Battery voltage sag under load.
   - Rapid Battery Drain rate (depletes capacity 30x faster with 15A idle load).
   - LVC latching (failsafe trigger does not clear LVC chatter; LVC remains latched even when OCV bounces back above 33V).
   - Recharge clears LVC and restores capacity to 10.0 Ah.
4. Run the unit tests by executing the file under Node.js, and verify all tests pass.
5. Write your handoff report to `c:\Users\thatw\Documents\Apollo-8\DashBoard\.agents\worker_milestone2\handoff.md`.
