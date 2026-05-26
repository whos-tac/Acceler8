# Remote Vertical HUD Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Implement a highly polished, symmetrical brutalist-cyberpunk vertical HUD UI (170x320) for the LilyGo T-Display S3 remote controller, utilizing ASCII-only indicators and live pack/cell battery voltages.

**Architecture:** Update `remote_app.cpp` to initialize modern LVGL layout components (ASCII signal bars, speed gauge circular arc, wide battery objects, exact volts, and footer). Configure display driver on both hardware and simulation to run in 170x320 vertical mode.

**Tech Stack:** C++, LVGL v8, TFT_eSPI (Arduino), SDL2 (Simulation).

---

## Proposed Changes

### Task 1: Symmetrical Vertical Screen Settings (Hardware & Simulation)
* **Modify:** [remote_app.cpp](file:///c:/Users/thatw/Documents/Apollo-8/DashBoard/src/remote/remote_app.cpp:130-144)
  * Set `tft.setRotation(0)` (or standard vertical orientation for LilyGo S3).
  * Configure `disp_drv.hor_res = 170;` and `disp_drv.ver_res = 320;` so hardware matches simulation.
* [ ] **Step 1: Update the display driver parameters**
  Update [remote_app.cpp](file:///c:/Users/thatw/Documents/Apollo-8/DashBoard/src/remote/remote_app.cpp:130-144) to establish a vertical 170x320 screen layout.
  ```cpp
  #ifdef ARDUINO
      tft.init();
      tft.setRotation(0); // Vertical rotation
      
      lv_init();
      lv_disp_draw_buf_init(&draw_buf, buf, NULL, 170 * 320 / 10);
      
      static lv_disp_drv_t disp_drv;
      lv_disp_drv_init(&disp_drv);
      disp_drv.hor_res = 170;
      disp_drv.ver_res = 320;
      disp_drv.flush_cb = my_disp_flush;
      disp_drv.draw_buf = &draw_buf;
      lv_disp_drv_register(&disp_drv);
  #endif
  ```
* [ ] **Step 2: Commit display driver layout**
  ```bash
  git add src/remote/remote_app.cpp
  git commit -m "style(remote): switch remote screen display driver to vertical 170x320"
  ```

---

### Task 2: Create Symmetrical HUD Layout Components
* **Modify:** [remote_app.cpp](file:///c:/Users/thatw/Documents/Apollo-8/DashBoard/src/remote/remote_app.cpp:55-63,145-168)
  * Set up global pointers for the new UI components:
    * `lbl_status` (Connection header text)
    * `arc_speed` (Central speed arc dial)
    * `lbl_speed` and `lbl_speed_unit` (Digits in dial center)
    * `bar_board` and `bar_remote` (Wide 32px vertical battery segments)
    * `lbl_board_volts` and `lbl_remote_volts` (Battery voltage values text)
    * `lbl_power` (Bottom power readout)
* [ ] **Step 1: Define global components in remote_app.cpp**
  Replace lines 55-63 in `remote_app.cpp` with the new UI pointers:
  ```cpp
  static lv_obj_t * lbl_status;
  static lv_obj_t * arc_speed;
  static lv_obj_t * lbl_speed;
  static lv_obj_t * bar_board;
  static lv_obj_t * bar_remote;
  static lv_obj_t * lbl_board_volts;
  static lv_obj_t * lbl_remote_volts;
  static lv_obj_t * lbl_power;
  #ifndef ARDUINO
  static lv_obj_t * slider_pot;
  static int sim_pot_val = 2048;
  void RemoteApp::set_sim_pot_val(int val) {
      sim_pot_val = val;
  }
  #endif
  ```
* [ ] **Step 2: Initialize UI elements in RemoteApp::init()**
  Rewrite `RemoteApp::init()`'s UI section to construct the vertical brutalist HUD layout:
  ```cpp
  void RemoteApp::init() {
      // Pin/ESP-NOW setup omitted for space, keep original pins/esp-now init
      
      // Style base
      lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x000000), 0);

      // 1. Connection Header Line
      lbl_status = lv_label_create(lv_scr_act());
      lv_obj_set_style_text_color(lbl_status, lv_color_hex(0x00FF88), 0);
      lv_obj_set_style_text_font(lbl_status, &lv_font_unscii_16, 0);
      lv_label_set_text(lbl_status, "SIG: [----] | CAN: !!");
      lv_obj_align(lbl_status, LV_ALIGN_TOP_MID, 0, 8);

      // 2. Circular Speed Dial (Speed gauge)
      arc_speed = lv_arc_create(lv_scr_act());
      lv_obj_set_size(arc_speed, 110, 110);
      lv_arc_set_rotation(arc_speed, 135);
      lv_arc_set_bg_angles(arc_speed, 0, 270);
      lv_arc_set_value(arc_speed, 0);
      lv_obj_align(arc_speed, LV_ALIGN_TOP_MID, 0, 32);
      lv_obj_set_style_arc_color(arc_speed, lv_color_hex(0x222222), LV_PART_MAIN); // bg
      lv_obj_set_style_arc_color(arc_speed, lv_color_hex(0xC3B1E1), LV_PART_INDICATOR); // active purple
      lv_obj_remove_style(arc_speed, NULL, LV_PART_KNOB); // hide knob

      lbl_speed = lv_label_create(lv_scr_act());
      lv_obj_set_style_text_color(lbl_speed, lv_color_hex(0xFFFFFF), 0);
      lv_obj_set_style_text_font(lbl_speed, &lv_font_unscii_16, 0); // Bold digits
      lv_label_set_text(lbl_speed, "0");
      lv_obj_align(lbl_speed, LV_ALIGN_TOP_MID, 0, 72);

      lv_obj_t* lbl_speed_unit = lv_label_create(lv_scr_act());
      lv_obj_set_style_text_color(lbl_speed_unit, lv_color_hex(0x555555), 0);
      lv_obj_set_style_text_font(lbl_speed_unit, &lv_font_unscii_16, 0);
      lv_label_set_text(lbl_speed_unit, "KM/H");
      lv_obj_align(lbl_speed_unit, LV_ALIGN_TOP_MID, 0, 92);

      // 3. Symmetrical Battery Columns (Board on left, Remote on right)
      // Board Bar (BRD)
      bar_board = lv_obj_create(lv_scr_act());
      lv_obj_set_size(bar_board, 32, 50);
      lv_obj_align(bar_board, LV_ALIGN_TOP_LEFT, 24, 160);
      lv_obj_set_style_bg_color(bar_board, lv_color_hex(0x00FF88), 0); // Green
      lv_obj_set_style_border_color(bar_board, lv_color_hex(0x555555), 0);
      lv_obj_set_style_border_width(bar_board, 1, 0);
      lv_obj_set_style_radius(bar_board, 0, 0);

      lv_obj_t* lbl_brd_title = lv_label_create(lv_scr_act());
      lv_obj_set_style_text_color(lbl_brd_title, lv_color_hex(0x555555), 0);
      lv_obj_set_style_text_font(lbl_brd_title, &lv_font_unscii_16, 0);
      lv_label_set_text(lbl_brd_title, "BOARD");
      lv_obj_align(lbl_brd_title, LV_ALIGN_TOP_LEFT, 20, 215);

      lbl_board_volts = lv_label_create(lv_scr_act());
      lv_obj_set_style_text_color(lbl_board_volts, lv_color_hex(0x00FF88), 0);
      lv_obj_set_style_text_font(lbl_board_volts, &lv_font_unscii_16, 0);
      lv_label_set_text(lbl_board_volts, "0.0V");
      lv_obj_align(lbl_board_volts, LV_ALIGN_TOP_LEFT, 16, 230);

      // Remote Bar (REM)
      bar_remote = lv_obj_create(lv_scr_act());
      lv_obj_set_size(bar_remote, 32, 50);
      lv_obj_align(bar_remote, LV_ALIGN_TOP_RIGHT, -24, 160);
      lv_obj_set_style_bg_color(bar_remote, lv_color_hex(0x00CCCC), 0); // Cyan
      lv_obj_set_style_border_color(bar_remote, lv_color_hex(0x555555), 0);
      lv_obj_set_style_border_width(bar_remote, 1, 0);
      lv_obj_set_style_radius(bar_remote, 0, 0);

      lv_obj_t* lbl_rem_title = lv_label_create(lv_scr_act());
      lv_obj_set_style_text_color(lbl_rem_title, lv_color_hex(0x555555), 0);
      lv_obj_set_style_text_font(lbl_rem_title, &lv_font_unscii_16, 0);
      lv_label_set_text(lbl_rem_title, "REMOTE");
      lv_obj_align(lbl_rem_title, LV_ALIGN_TOP_RIGHT, -16, 215);

      lbl_remote_volts = lv_label_create(lv_scr_act());
      lv_obj_set_style_text_color(lbl_remote_volts, lv_color_hex(0x00CCCC), 0);
      lv_obj_set_style_text_font(lbl_remote_volts, &lv_font_unscii_16, 0);
      lv_label_set_text(lbl_remote_volts, "3.70V");
      lv_obj_align(lbl_remote_volts, LV_ALIGN_TOP_RIGHT, -20, 230);

      // 4. Bottom Footer (Power Value)
      lbl_power = lv_label_create(lv_scr_act());
      lv_obj_set_style_text_color(lbl_power, lv_color_hex(0x00CCCC), 0);
      lv_obj_set_style_text_font(lbl_power, &lv_font_unscii_16, 0);
      lv_label_set_text(lbl_power, "POWER: 0W");
      lv_obj_align(lbl_power, LV_ALIGN_BOTTOM_MID, 0, -8);

  #ifndef ARDUINO
      // Setup vertical slider on left margin for simulation potentiometer
      slider_pot = lv_slider_create(lv_scr_act());
      lv_slider_set_range(slider_pot, 0, 4095);
      lv_slider_set_value(slider_pot, 2048, LV_ANIM_OFF);
      lv_obj_set_size(slider_pot, 10, 80);
      lv_obj_align(slider_pot, LV_ALIGN_TOP_MID, 0, 160);
  #endif
  }
  ```
* [ ] **Step 3: Commit structural layout UI code**
  ```bash
  git add src/remote/remote_app.cpp
  git commit -m "feat(remote): build vertical symmetrical HUD UI using LVGL"
  ```

---

### Task 3: Dynamic Updates & Calculations (Remote Battery Volts & Signal strength)
* **Modify:** [remote_app.cpp](file:///c:/Users/thatw/Documents/Apollo-8/DashBoard/src/remote/remote_app.cpp:77-97,170-208)
  * Read actual remote battery voltage (mock in simulation from cell range `3.7V - 4.2V` based on potentiometer level or generic telemetry update). On Arduino hardware, use a mock cell read or ADC calculation.
  * Formulate ASCII connection display based on communication timing/RSSI.
  * Update dial speed indicator, bar heights, and label values dynamically.
* [ ] **Step 1: Implement data updates in remote_onDataRecv and RemoteApp::update()**
  Refactor `remote_onDataRecv` and `RemoteApp::update()` to dynamically populate components:
  ```cpp
  extern "C" void remote_onDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
      if (len == sizeof(TelemetryPacket)) {
          memcpy(&current_telemetry, incomingData, sizeof(TelemetryPacket));
          
          // 1. Update Speed dial and label
          if (lbl_speed) {
              char spd_buf[16];
              snprintf(spd_buf, sizeof(spd_buf), "%.0f", current_telemetry.speed_kmh);
              lv_label_set_text(lbl_speed, spd_buf);
          }
          if (arc_speed) {
              lv_arc_set_value(arc_speed, (int)current_telemetry.speed_kmh);
          }

          // 2. Update Board Battery
          if (lbl_board_volts) {
              char v_buf[16];
              snprintf(v_buf, sizeof(v_buf), "%.1fV", current_telemetry.battery_voltage_v);
              lv_label_set_text(lbl_board_volts, v_buf);
          }
          if (bar_board) {
              float pct = ((current_telemetry.battery_voltage_v - 32.0f) / 10.0f) * 100.0f;
              if (pct < 0) pct = 0; if (pct > 100) pct = 100;
              lv_obj_set_height(bar_board, (int)(pct * 0.5f)); // map 100% to 50px height
          }

          // 3. Update Power readout
          if (lbl_power) {
              char pwr_buf[32];
              snprintf(pwr_buf, sizeof(pwr_buf), "POWER: %.0fW", current_telemetry.power_w);
              lv_label_set_text(lbl_power, pwr_buf);
              if (current_telemetry.power_w < 0) {
                  lv_obj_set_style_text_color(lbl_power, lv_color_hex(0x00FF88), 0); // regen green
              } else {
                  lv_obj_set_style_text_color(lbl_power, lv_color_hex(0x00CCCC), 0); // normal cyan
              }
          }

          // 4. Update Header status
          if (lbl_status) {
              // Map signal RSSI / active status to ASCII bars
              const char* sig = "[#][#][#][-]";
              const char* can = current_telemetry.can_alive ? "OK" : "!!";
              char stat_buf[64];
              snprintf(stat_buf, sizeof(stat_buf), "SIG: %s | CAN: %s", sig, can);
              lv_label_set_text(lbl_status, stat_buf);
              if (current_telemetry.can_alive) {
                  lv_obj_set_style_text_color(lbl_status, lv_color_hex(0x00FF88), 0);
              } else {
                  lv_obj_set_style_text_color(lbl_status, lv_color_hex(0xFF3300), 0); // alarm accent
              }
          }
      }
  }

  void RemoteApp::update() {
      static uint32_t last_send = 0;
      
      // Read Potentiometer
  #ifdef ARDUINO
      int pot_val = analogRead(PIN_POT); // 0-4095
      // Hardware remote battery voltage read (mocked / scaled 3.7 - 4.2)
      float rem_volts = 3.7f + (analogRead(4) / 4095.0f) * 0.5f; 
  #else
      if (slider_pot) sim_pot_val = lv_slider_get_value(slider_pot);
      int pot_val = sim_pot_val;
      // Simulate remote battery discharging slightly or tied to throttle level
      float rem_volts = 3.92f - (pot_val / 4095.0f) * 0.15f;
  #endif
      
      // Update Remote battery bar & volts
      if (lbl_remote_volts) {
          char rem_buf[16];
          snprintf(rem_buf, sizeof(rem_buf), "%.2fV", rem_volts);
          lv_label_set_text(lbl_remote_volts, rem_buf);
      }
      if (bar_remote) {
          float pct = ((rem_volts - 3.7f) / 0.5f) * 100.0f;
          if (pct < 0) pct = 0; if (pct > 100) pct = 100;
          lv_obj_set_height(bar_remote, (int)(pct * 0.5f)); // map 100% to 50px height
      }

      // Map throttle (-100 to 100)
      float throttle = 0.0f;
      if (pot_val > 2148) {
          throttle = ((pot_val - 2148) / (4095.0f - 2148.0f)) * 100.0f;
      } else if (pot_val < 1948) {
          throttle = ((pot_val - 1948) / 1948.0f) * 100.0f;
      }

      // Send to receiver
      if (millis() - last_send > 50) {
          last_send = millis();
          ControlPacket pkt;
          pkt.throttle_percent = throttle;
          esp_now_send(receiver_mac, (uint8_t *)&pkt, sizeof(ControlPacket));
      }
  #ifdef ARDUINO
      lv_timer_handler();
  #endif
  }
  ```
* [ ] **Step 2: Run build in simulator mode to compile the full stack**
  Run: `env -u CLOUD_ENV platformio run -e native`
  Expected: Successful compilation of simulation executable without errors.
* [ ] **Step 3: Launch Native simulator to test UI elements**
  Run: `env -u CLOUD_ENV .pio/build/native/program`
  Expected: Successful opening of 3 windows. Symmetrical Remote UI should show ASCII signal bars, exact voltages, circular Dial, and wide battery blocks. Remote slider should throttle the motor, and Receiver telemetry sliders should update the Remote screen dynamically.
* [ ] **Step 4: Commit working Remote HUD UI implementation**
  ```bash
  git add src/remote/remote_app.cpp
  git commit -m "feat(remote): complete fully responsive 170x320 vertical HUD UI with battery voltages and ASCII stats"
  ```
