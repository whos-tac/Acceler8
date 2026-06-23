## 2026-06-23T08:02:21+02:00
Review the redesigned digital test environment implementation.
Verify the correctness, completeness, robustness, and interface conformance of the code.
Run the compile and test commands to verify:
`pio run -e native_tests`
`.pio\build\native_tests\program.exe`
`pio run -e native_full_stack`
Check that everything is built in /ponytail mode: the minimum that works, avoid unrequested abstractions, and mark simplifications with a `// ponytail:` comment.
Provide a detailed verification report with test output.

## 2026-06-23T08:02:21Z
You are a teamwork_preview_challenger. Empirically verify correctness of the physics engine, battery sag, and failsafe timing.
Run the test suite `pio run -e native_tests` and inspect the test executable `.pio\build\native_tests\program.exe`.
Determine if there are boundary edge cases (division by zero, floats, etc.) not handled in the core logic.
Verify that the layout matches what is specified in PROJECT.md.
Document your findings.

