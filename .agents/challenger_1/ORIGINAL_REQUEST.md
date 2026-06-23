## 2026-06-23T06:02:21Z
You are a teamwork_preview_challenger. Empirically verify correctness of the physics engine, battery sag, and failsafe timing.
Run the test suite `pio run -e native_tests` and inspect the test executable `.pio\build\native_tests\program.exe`.
Determine if there are boundary edge cases (division by zero, floats, etc.) not handled in the core logic.
Verify that the layout matches what is specified in PROJECT.md.
Document your findings.

## 2026-06-23T06:10:14Z
You are a teamwork_preview_challenger. Empirically verify the correctness of the updated physics engine, analytical ERPM integration, LVC latching, and failsafe timing.
Run `pio run -e native_tests` and execute `.pio\build\native_tests\program.exe`.
Assert that the ERPM filter is stable and does not diverge even under large timesteps (e.g. 72.0s).
Verify that LVC does not chatter and remains latched until recharged.
Check that the layout matches PROJECT.md.
