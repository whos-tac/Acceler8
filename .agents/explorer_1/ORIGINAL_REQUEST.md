## 2026-06-22T15:45:12Z

Verify the build environment for native full stack by compiling it via PlatformIO (`pio run -e native_full_stack`).
Explore the codebase to understand the SDL window setup in `src/simulation/sim_main.cpp`, how mouse events are read/sent to LVGL, and how `RemoteApp`, `DashApp`, and `ReceiverApp` initialize.
Propose a simple design for:
1. Combining the three virtual displays into a single SDL window (approx 1024x600) with split-screen panels (Remote, Dash, Controls) and translating mouse coordinates.
2. Integrating a physical ESC and battery model inside the simulation (receiving the Receiver's UART packets and feeding back telemetry: ERPM, current, duty cycle, voltage).
3. Simulating communication loss failsafe and battery drain.
4. Setting up a command-line C++ unit test suite to test the core logic (throttle mapping, failsafe, battery drain) independently of the UI.
Remember to operate in /ponytail mode: propose the minimum that works, avoid unrequested abstractions, and plan to mark simplifications with a `// ponytail:` comment. Do NOT modify any code. Only explore, run checks, and report.
