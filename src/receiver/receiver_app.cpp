#include "receiver_app.h"
#ifdef ARDUINO
#include <Arduino.h>
#endif
#include "../espnow_packets.h"
#include "esc_uart_driver.h"
#include "espnow_receiver.h"
#include <cmath>
#include <cstdio>
#include <cstring>
#include <lvgl.h>

#ifndef ARDUINO
extern "C" uint32_t millis();
#endif

// ── Safety Configuration ──────────────────────────────────────────────
#define MAX_DRIVE_CURRENT_A 50.0f // Max forward drive current (Amps)
#define MAX_BRAKE_CURRENT_A                                                    \
  50.0f // Max regen brake current (Amps) — same as drive
#define THROTTLE_DEADZONE 10.0f // ±10% deadzone at center
#define RAMP_RATE_PER_SEC 75.0f // Max throttle change %/sec (0→100% in ~1.3s)
#define RAMP_DOWN_RATE_PER_SEC                                                 \
  500.0f // Fast decay %/sec when decelerating or braking
#define FAILSAFE_COAST_RATE                                                    \
  200.0f // Throttle decay %/sec on signal loss (100→0% in ~0.5s)
#define FAILSAFE_TIMEOUT_MS 250 // ms before connection is considered lost
#define UART_UPDATE_MS 50       // UART command send interval (20Hz)

// Throttle state
static float ramped_throttle = 0.0f; // Smoothed output after ramp limiter

#ifndef ARDUINO
#include "../simulation/esc_model.h"
#include "mechanical_config.h"

extern bool sim_comm_loss;
extern uint8_t sim_remote_btn_state;

static SimCore::SimState sim_state;
static bool sim_rapid_drain = false;

static lv_obj_t *lbl_receiver_throttle;
static lv_obj_t *chart_esc;
static lv_chart_series_t *ser_motor_current;
static lv_chart_series_t *ser_erpm;
static lv_chart_series_t *ser_duty;

static void cb_comm_loss_event_handler(lv_event_t *e) {
  lv_obj_t *cb = lv_event_get_target(e);
  sim_comm_loss = lv_obj_has_state(cb, LV_STATE_CHECKED);
}

static void cb_rapid_drain_event_handler(lv_event_t *e) {
  lv_obj_t *cb = lv_event_get_target(e);
  sim_rapid_drain = lv_obj_has_state(cb, LV_STATE_CHECKED);
}

static void btn_recharge_event_handler(lv_event_t *e) {
  sim_state.capacity_ah = 10.0f;
  sim_state.lvc_active = false;
}

static void btn_up_event_handler(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);
  if (code == LV_EVENT_PRESSED) {
    sim_remote_btn_state |= (1 << 0);
  } else if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
    sim_remote_btn_state &= ~(1 << 0);
  }
}
static void btn_down_event_handler(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);
  if (code == LV_EVENT_PRESSED) {
    sim_remote_btn_state |= (1 << 1);
  } else if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
    sim_remote_btn_state &= ~(1 << 1);
  }
}
static void btn_left_event_handler(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);
  if (code == LV_EVENT_PRESSED) {
    sim_remote_btn_state |= (1 << 2);
  } else if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
    sim_remote_btn_state &= ~(1 << 2);
  }
}
static void btn_right_event_handler(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);
  if (code == LV_EVENT_PRESSED) {
    sim_remote_btn_state |= (1 << 3);
  } else if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
    sim_remote_btn_state &= ~(1 << 3);
  }
}
static void btn_confirm_event_handler(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);
  if (code == LV_EVENT_PRESSED) {
    sim_remote_btn_state |= (1 << 4);
  } else if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
    sim_remote_btn_state &= ~(1 << 4);
  }
}
#endif

void ReceiverApp::init() {
#ifdef ARDUINO
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH); // Active LOW on D1 Mini
#endif
  EscUartDriver::init();
  EspnowReceiver::init();

#ifndef ARDUINO
  // Native LVGL setup - Control Panel
  lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x1e1e1e), 0);

  // Label for real-time status text
  lbl_receiver_throttle = lv_label_create(lv_scr_act());
  lv_obj_set_style_text_color(lbl_receiver_throttle, lv_color_hex(0xFFFFFF), 0);
  lv_obj_align(lbl_receiver_throttle, LV_ALIGN_TOP_LEFT, 10, 10);
  lv_label_set_text(lbl_receiver_throttle,
                    "Simulator Status:\nInitializing...");

  // Checkboxes
  lv_obj_t *cb_comm = lv_checkbox_create(lv_scr_act());
  lv_checkbox_set_text(cb_comm, "Comm Loss");
  lv_obj_set_style_text_color(cb_comm, lv_color_hex(0xFFFFFF), 0);
  lv_obj_align(cb_comm, LV_ALIGN_TOP_LEFT, 10, 110);
  lv_obj_add_event_cb(cb_comm, cb_comm_loss_event_handler,
                      LV_EVENT_VALUE_CHANGED, NULL);
  if (sim_comm_loss)
    lv_obj_add_state(cb_comm, LV_STATE_CHECKED);

  lv_obj_t *cb_drain = lv_checkbox_create(lv_scr_act());
  lv_checkbox_set_text(cb_drain, "Rapid Drain");
  lv_obj_set_style_text_color(cb_drain, lv_color_hex(0xFFFFFF), 0);
  lv_obj_align(cb_drain, LV_ALIGN_TOP_LEFT, 170, 110);
  lv_obj_add_event_cb(cb_drain, cb_rapid_drain_event_handler,
                      LV_EVENT_VALUE_CHANGED, NULL);

  // Recharge Button
  lv_obj_t *btn_recharge = lv_btn_create(lv_scr_act());
  lv_obj_set_size(btn_recharge, 300, 35);
  lv_obj_align(btn_recharge, LV_ALIGN_TOP_LEFT, 10, 150);
  lv_obj_add_event_cb(btn_recharge, btn_recharge_event_handler,
                      LV_EVENT_CLICKED, NULL);
  lv_obj_t *lbl_recharge = lv_label_create(btn_recharge);
  lv_label_set_text(lbl_recharge, "Recharge Battery");
  lv_obj_center(lbl_recharge);

  // Remote D-pad simulation
  auto make_dpad_btn = [](const char *text, int x, int y, lv_event_cb_t cb) {
    lv_obj_t *btn = lv_btn_create(lv_scr_act());
    lv_obj_set_size(btn, 65, 32);
    lv_obj_align(btn, LV_ALIGN_TOP_LEFT, x, y);
    lv_obj_add_event_cb(btn, cb, LV_EVENT_ALL, NULL);
    lv_obj_t *lbl = lv_label_create(btn);
    lv_label_set_text(lbl, text);
    lv_obj_center(lbl);
    return btn;
  };

  make_dpad_btn("UP", 127, 200, btn_up_event_handler);
  make_dpad_btn("LEFT", 57, 240, btn_left_event_handler);
  make_dpad_btn("OK", 127, 240, btn_confirm_event_handler);
  make_dpad_btn("RIGHT", 197, 240, btn_right_event_handler);
  make_dpad_btn("DOWN", 127, 280, btn_down_event_handler);

  // ESC Telemetry Chart
  chart_esc = lv_chart_create(lv_scr_act());
  lv_obj_set_size(chart_esc, 300, 235);
  lv_obj_align(chart_esc, LV_ALIGN_TOP_LEFT, 10, 330);
  lv_chart_set_type(chart_esc, LV_CHART_TYPE_LINE);
  lv_chart_set_range(chart_esc, LV_CHART_AXIS_PRIMARY_Y, 0, 800);

  ser_motor_current = lv_chart_add_series(chart_esc, lv_color_hex(0xFF0000),
                                          LV_CHART_AXIS_PRIMARY_Y); // Red
  ser_erpm = lv_chart_add_series(chart_esc, lv_color_hex(0x00FF00),
                                 LV_CHART_AXIS_PRIMARY_Y); // Green
  ser_duty = lv_chart_add_series(chart_esc, lv_color_hex(0x0000FF),
                                 LV_CHART_AXIS_PRIMARY_Y); // Blue

  for (int i = 0; i < 50; i++) {
    lv_chart_set_next_value(chart_esc, ser_motor_current, 0);
    lv_chart_set_next_value(chart_esc, ser_erpm, 0);
    lv_chart_set_next_value(chart_esc, ser_duty, 0);
  }
#endif
}

void ReceiverApp::update() {
  static uint32_t last_loop_ms = millis();
  uint32_t now = millis();
  float dt = (now - last_loop_ms) / 1000.0f;
  last_loop_ms = now;

  // ponytail: prevent division-by-zero or extremely high dt during
  // initialization/pauses
  if (dt <= 0.0f)
    dt = 0.001f;
  if (dt > 0.5f)
    dt = 0.1f;

#ifdef ARDUINO
  // ── Determine target throttle and ramp rate ──
  uint32_t last_remote_rx_ms;
  float current_throttle;
  uint8_t btn_state;
  noInterrupts();
  last_remote_rx_ms = EspnowReceiver::get_last_rx_ms();
  current_throttle = EspnowReceiver::get_latest_throttle();
  btn_state = EspnowReceiver::get_latest_button_state();
  bool settings_active = EspnowReceiver::is_settings_active();
  uint8_t current_gear = EspnowReceiver::get_gear();
  uint8_t current_direction = EspnowReceiver::get_direction();
  interrupts();

  static bool first_packet_received = false;
  if (last_remote_rx_ms != 0) {
    first_packet_received = true;
  }

  if (std::isnan(current_throttle)) {
    current_throttle = 0.0f;
  }
  if (current_throttle > 100.0f)
    current_throttle = 100.0f;
  if (current_throttle < -100.0f)
    current_throttle = -100.0f;

  bool signal_lost = (!first_packet_received ||
                      (now - last_remote_rx_ms > FAILSAFE_TIMEOUT_MS));
  float target;

  if (signal_lost) {
    digitalWrite(LED_BUILTIN, HIGH); // LED OFF when disconnected
  } else {
    if (std::abs(current_throttle) > THROTTLE_DEADZONE) {
      digitalWrite(LED_BUILTIN, (now % 50 < 25) ? LOW : HIGH); // 20Hz blink
    } else {
      digitalWrite(LED_BUILTIN, (now % 200 < 100) ? LOW : HIGH); // 5Hz blink
    }
  }

  static bool safe_start = false;
  if (signal_lost) {
    safe_start = false;
  }
  if (!signal_lost && std::abs(current_throttle) <= THROTTLE_DEADZONE) {
    safe_start = true;
  }

  if (signal_lost || settings_active || !safe_start) {
    target = 0.0f;
  } else {
    if (std::abs(current_throttle) <= THROTTLE_DEADZONE) {
      target = 0.0f;
    } else {
      float sign = (current_throttle > 0.0f) ? 1.0f : -1.0f;
      target = sign * (std::abs(current_throttle) - THROTTLE_DEADZONE) *
               (100.0f / (100.0f - THROTTLE_DEADZONE));
    }
    if (target < 0.0f) {
      target *= (MAX_BRAKE_CURRENT_A / MAX_DRIVE_CURRENT_A);
    }
  }

  float time_left = dt;
  while (time_left > 0.0f && ramped_throttle != target) {
    float current_rate;
    if (signal_lost || settings_active || !safe_start) {
      current_rate = FAILSAFE_COAST_RATE;
    } else {
      bool accelerating = false;
      if (target > 0.0f && target > ramped_throttle &&
          ramped_throttle >= 0.0f) {
        accelerating = true;
      } else if (target < 0.0f && target < ramped_throttle &&
                 ramped_throttle <= 0.0f) {
        accelerating = true;
      }
      current_rate = accelerating ? RAMP_RATE_PER_SEC : RAMP_DOWN_RATE_PER_SEC;
    }

    float max_delta = current_rate * time_left;

    if (ramped_throttle < target) {
      if (ramped_throttle < 0.0f && target >= 0.0f) {
        float dist = 0.0f - ramped_throttle;
        if (max_delta > dist) {
          ramped_throttle = 0.0f;
          time_left -= dist / current_rate;
          continue;
        }
      }
      ramped_throttle += max_delta;
      if (ramped_throttle > target)
        ramped_throttle = target;
      break;
    } else {
      if (ramped_throttle > 0.0f && target <= 0.0f) {
        float dist = ramped_throttle - 0.0f;
        if (max_delta > dist) {
          ramped_throttle = 0.0f;
          time_left -= dist / current_rate;
          continue;
        }
      }
      ramped_throttle -= max_delta;
      if (ramped_throttle < target)
        ramped_throttle = target;
      break;
    }
  }

  float output = ramped_throttle;

  static uint32_t last_uart = 0;
  static uint32_t last_keep_alive = 0;
  if (now - last_uart > UART_UPDATE_MS) {
    last_uart = now;
    int32_t raw_val = 512 + (int32_t)(output * 5.12f);
    if (raw_val > 1023)
      raw_val = 1023;
    if (raw_val < 0)
      raw_val = 0;
    uint16_t throttle_val = (uint16_t)raw_val;
    bool horn_active = !settings_active && ((btn_state & (1 << 4)) != 0);
    bool headlight_active = EspnowReceiver::is_headlight_active();
    bool brake_light_active = (target < 0.0f);
    EscUartDriver::send_throttle(throttle_val, current_gear, current_direction,
                                 horn_active, headlight_active,
                                 brake_light_active);
  }

  if (now - last_keep_alive >= 200) {
    last_keep_alive = now;
    EscUartDriver::send_keep_alive();
  }

  static uint32_t last_status_send = 0;
  if (now - last_status_send > 100) {
    last_status_send = now;
    EspnowReceiver::send_status_to_dash(signal_lost);
  }
#else
  // ── NATIVE SIMULATOR CONTROLS UPDATE ──
  uint32_t last_remote_rx_ms = EspnowReceiver::get_last_rx_ms();
  float current_throttle = EspnowReceiver::get_latest_throttle();
  if (std::isnan(current_throttle)) {
    current_throttle = 0.0f;
  }
  bool settings_active = EspnowReceiver::is_settings_active();

  bool signal_lost = (last_remote_rx_ms == 0 ||
                      (now - last_remote_rx_ms > FAILSAFE_TIMEOUT_MS));

  // Update ramped throttle
  SimCore::calculate_ramped_throttle(current_throttle,
                                     sim_state.ramped_throttle, dt, signal_lost,
                                     settings_active, sim_state.safe_start);

  // Update physics simulation
  SimCore::update_esc_physics(sim_state, dt, sim_rapid_drain);

  // Update status labels
  if (lbl_receiver_throttle) {
    char status_str[384];
    snprintf(status_str, sizeof(status_str),
             "State: %s %s\n"
             "Input: %.1f%%  Ramped: %.1f%%\n"
             "Speed: %.1f km/h  ERPM: %.0f\n"
             "Motor: %.1f A  Batt: %.1f A\n"
             "Voltage: %.2f V (Sag: %.2fV)\n"
             "Power: %.1f W  Cap: %.2f Ah",
             sim_state.lvc_active ? "LVC" : "Normal",
             signal_lost ? "[NO SIGNAL]" : "OK", current_throttle,
             sim_state.ramped_throttle, calculate_speed_kmh(sim_state.erpm),
             sim_state.erpm, sim_state.motor_current, sim_state.battery_current,
             sim_state.battery_voltage, sim_state.voltage_sag,
             sim_state.power_w, sim_state.capacity_ah);
    lv_label_set_text(lbl_receiver_throttle, status_str);
  }

  // Push values to the chart series
  if (chart_esc) {
    // Scale values to fit in the 0-800 chart range
    // Motor Current: scaled by 10 (0 to 500)
    // ERPM: divided by 100 (0 to 800)
    // Duty cycle %: scaled by 8 (0 to 800)
    lv_chart_set_next_value(chart_esc, ser_motor_current,
                            (int)(sim_state.motor_current * 10.0f));
    lv_chart_set_next_value(chart_esc, ser_erpm,
                            (int)(sim_state.erpm / 100.0f));
    lv_chart_set_next_value(chart_esc, ser_duty,
                            (int)(std::abs(sim_state.ramped_throttle) * 8.0f));
    lv_chart_refresh(chart_esc);
  }

  // Send Status Packet to Dash
  static uint32_t last_status_send = 0;
  if (now - last_status_send > 100) {
    last_status_send = now;
    EspnowReceiver::send_status_to_dash(signal_lost);
  }

  // Send Telemetry Packet to Dash
  static uint32_t last_telemetry = 0;
  if (now - last_telemetry > 100) {
    last_telemetry = now;
    float speed = calculate_speed_kmh(sim_state.erpm);
    EspnowReceiver::send_mock_telemetry(speed, sim_state.battery_voltage,
                                        sim_state.power_w,
                                        sim_state.battery_current);
  }
#endif
}
