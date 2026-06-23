# Web Simulator Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build a self-contained, browser-based clone of the Apollo-8 / ACCELER8 native simulator in HTML + vanilla JS under `web_sim/`, replicating the Remote, Dashboard, and Control panels with a fully functional ESC physics model, telemetry chart, and input controls.

**Architecture:** A single-page web application (`index.html`) using vanilla HTML, CSS, and JS (with no external dependencies or build steps) that runs the simulation loop at ~20Hz (50ms interval). The ESC physics model will be a self-contained JS module/class. Telemetry charts and UI gauges will be custom-drawn using HTML5 Canvas to avoid external CDN dependencies.

**Tech Stack:** HTML5, CSS3, Vanilla ES6 JavaScript (HTML5 Canvas for rendering gauges and scrolling charts).

---

### Task 1: Codebase Exploration & Physics Extraction

**Files:**
- Read: `src/simulation/esc_model.h` / `esc_model.cpp`
- Read: `src/receiver/receiver_app.cpp`
- Read: `src/remote/remote_app.cpp`
- Read: `src/dash_app.cpp`
- Create: `.agents/orchestrator/physics_specs.md`

- [ ] **Step 1: Inspect the ESC physics model code**
  - Read `src/simulation/esc_model.h` and `esc_model.cpp`.
  - Extract the exact state properties, constants, and math for:
    - Throttle ramping (input throttle vs. ramped throttle)
    - Motor ERPM calculation and linear filter / integration
    - Motor current and battery current calculation (including voltage sag)
    - Battery capacity drain and rapid battery drain
    - Low-Voltage Cutoff (LVC) state machine (specifically noting the LVC chatter/latch bug fix: LVC must latch and not reset when OCV >= 33.0V if it has already triggered, until recharge)
- [ ] **Step 2: Inspect UI specifications**
  - Read `src/remote/remote_app.cpp` to understand the throttle arc rendering and D-pad display.
  - Read `src/dash_app.cpp` to extract wheel configuration (diameter, gear ratio) and the speed/power calculation formulas.
  - Read `src/receiver/receiver_app.cpp` to extract the chart details (scrolling telemetry data points, D-pad layout, button mappings).
- [ ] **Step 3: Document findings**
  - Save all math formulas, constants, and UI layouts to `.agents/orchestrator/physics_specs.md`.

---

### Task 2: ESC Physics Core and Test Suite

**Files:**
- Create: `web_sim/esc_model.js` (Self-contained JS class for the ESC physics)
- Create: `web_sim/test_physics.js` (Node-based unit test suite for physics logic)

- [ ] **Step 1: Write the unit tests for JS physics model**
  - Implement tests in `web_sim/test_physics.js` that check:
    - Ramped throttle behavior (throttle updates slowly, no instant jumps)
    - Failsafe triggers (coasts to 0 throttle when signal lost)
    - Battery voltage sag under load
    - Rapid battery drain rate
    - LVC activation and proper latching (LVC does not chatter or release when load is removed and OCV goes above 33V)
    - Recharge button behavior (resets voltage to 42.0V and clears LVC latch)
- [ ] **Step 2: Implement the ESC model in JavaScript**
  - Write `web_sim/esc_model.js` matching the C++ logic exactly, including the fixed LVC latch.
- [ ] **Step 3: Run the unit tests**
  - Command: `node web_sim/test_physics.js`
  - Expected: All physics tests pass.

---

### Task 3: HTML Structure and Visual Panels

**Files:**
- Create: `web_sim/index.html`
- Create: `web_sim/style.css`

- [ ] **Step 1: Set up HTML structure with three panels**
  - Middle: Dashboard panel (480x480 container)
  - Left: Remote panel (170x320 container)
  - Right: Control Panel (320x580 container)
  - Add borders, titles, and layout styling matching the native layout.
- [ ] **Step 2: Add interactive control elements to the Control Panel**
  - Throttle slider (-100% to +100%)
  - "Comm Loss" toggle checkbox
  - "Rapid Battery Drain" toggle checkbox
  - "Recharge" button
  - D-Pad buttons (Up, Down, Left, Right, OK)
  - Canvas element for telemetry chart
- [ ] **Step 3: Add elements to Remote and Dashboard Panels**
  - Remote: Canvas for throttle arc and button indicator
  - Dashboard: Text/SVG displays for Speed (km/h), Voltage (V), and Power (W)

---

### Task 4: UI Canvas Renderers and Telemetry Chart

**Files:**
- Create: `web_sim/ui_renderers.js` (Canvas draw functions for chart and gauges)
- Modify: `web_sim/index.html` (Include the scripts)

- [ ] **Step 1: Implement the scrolling telemetry line chart**
  - Draw a scrolling grid using Canvas API.
  - Plot Motor Current, ERPM (scaled), and Battery Voltage in different colors.
  - Implement continuous chart scrolling at 20Hz.
- [ ] **Step 2: Implement Remote Panel throttle arc**
  - Draw the curved arc representing throttle level from -100% to +100% on the Remote canvas.
  - Draw active/inactive visual indicator for Remote buttons.
- [ ] **Step 3: Implement Dashboard Panel speedometer and gauges**
  - Draw speedometer gauge or formatted numeric speed (km/h), voltage sag, power (W) indicator.

---

### Task 5: Integration, Simulation Loop, and E2E Acceptance

**Files:**
- Create: `web_sim/app.js` (Simulation loop, state wiring, and input event handling)
- Modify: `web_sim/index.html` (Bootstrap code)

- [ ] **Step 1: Wire input events and loop execution**
  - Implement a 20Hz interval loop in `web_sim/app.js`.
  - On each tick, update the ESC physics model state.
  - Handle throttle slider input, button presses, toggles, and Recharge action.
  - Update all UI panels and redraw canvases.
- [ ] **Step 2: Implement Failsafe and Rapid Drain interactions**
  - Wire "Comm Loss" to drop remote packets and trigger receiver failsafe (coasting throttle to 0).
  - Wire "Rapid Battery Drain" to drain capacity rapidly when active.
  - Wire LVC to restrict throttle when battery is depleted.
- [ ] **Step 3: Verify all Acceptance Criteria**
  - Launch index.html in browser and test manually.
  - Check browser console logs for any errors.
  - Verify failsafe, LVC latching, and chart scrolling.
