#include "remote_app.h"
#ifdef ARDUINO
#include <Arduino.h>
#endif
#include "../espnow_packets.h"
#include <cstdio>
#include <lvgl.h>

#ifdef ARDUINO
#include <WiFi.h>
#include <esp_now.h>
#include <esp_sleep.h>
#else
// Mock ESP-NOW
#include <cstring>
typedef uint8_t esp_now_send_status_t;
#define ESP_NOW_SEND_SUCCESS 0
extern "C" void esp_now_send(const uint8_t *peer_addr, const uint8_t *data, size_t len);
extern "C" uint32_t millis();
extern "C" void delay(uint32_t ms);
#endif

// MAC Addresses
static uint8_t receiver_mac[] = {0xEC, 0x64, 0xC9, 0xCC, 0xD8, 0x54};
static uint8_t dash_mac[] = {0x3C, 0x0F, 0x02, 0xC2, 0xD4, 0xCC};

// Telemetry State
static TelemetryPacket current_telemetry = {0};

// Pin Definitions
#define PIN_POT 1
#define PIN_BTN_UP 2
#define PIN_BTN_DOWN 3
#define PIN_BTN_LEFT 10
#define PIN_BTN_RIGHT 11
#define PIN_BTN_CONFIRM 12
#define PIN_POWER_ON 15

#ifdef ARDUINO
#include <Preferences.h>
static Preferences preferences;
static int pot_min = 2048;
static int pot_max = 2048;
static uint32_t last_activity_time = 0;
static int last_pot_val = -1;
static uint8_t last_btn_state = 0;

#include <TFT_eSPI.h>
extern TFT_eSPI tft;

static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[170 * 320 / 10];

#ifdef ARDUINO
static void update_adc_calibration(int pot_val) {
    if (pot_val > 50 && pot_val < 4095) {
        if (pot_val < pot_min) {
            pot_min = pot_val;
            preferences.putInt("pot_min", pot_min);
        }
        if (pot_val > pot_max) {
            pot_max = pot_val;
            preferences.putInt("pot_max", pot_max);
        }
    }
}

static void check_inactivity_sleep(float throttle) {
    uint8_t current_btn_state = 0;
    const uint8_t pins[] = {PIN_BTN_UP, PIN_BTN_DOWN, PIN_BTN_LEFT, PIN_BTN_RIGHT, PIN_BTN_CONFIRM};
    for (int i = 0; i < 5; i++) {
        if (digitalRead(pins[i]) == LOW) {
            current_btn_state |= (1 << i);
        }
    }

    bool activity = false;
    if (current_btn_state != last_btn_state) {
        activity = true;
        last_btn_state = current_btn_state;
    }

    if (abs(throttle) > 2.0f) {
        activity = true;
    }

    if (activity) {
        last_activity_time = millis();
    }

    if (millis() - last_activity_time > 300000) { // 5 minutes
        Serial.println("Inactivity timeout. Deep sleep.");
        digitalWrite(PIN_POWER_ON, LOW);
        esp_sleep_enable_ext0_wakeup((gpio_num_t)PIN_BTN_CONFIRM, 0);
        esp_deep_sleep_start();
    }
}
#endif

static void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);
    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, w, h);
    tft.pushColors((uint16_t *)&color_p->full, w * h, true);
    tft.endWrite();
    lv_disp_flush_ready(disp);
}
#endif

// LVGL UI elements
static lv_obj_t * lbl_status;
static lv_obj_t * arc_speed;
static lv_obj_t * arc_throttle;
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

static void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
#ifdef DEBUG_ESPNOW
#ifdef ARDUINO
    if (status != ESP_NOW_SEND_SUCCESS) {
        Serial.println("[ESP-NOW] TX Remote -> Receiver FAILED");
    }
#else
    printf("[ESP-NOW] TX Remote -> Receiver status: %d\n", status);
#endif
#endif
}

extern "C" void remote_onDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
    if (len == sizeof(TelemetryPacket)) {
        memcpy(&current_telemetry, incomingData, sizeof(TelemetryPacket));
#ifdef DEBUG_ESPNOW
#ifdef ARDUINO
        Serial.printf("[ESP-NOW] RX Dash -> Remote | Speed: %.1f km/h | Batt: %.1fV | Power: %.0fW\n", 
                      current_telemetry.speed_kmh, current_telemetry.battery_voltage_v, current_telemetry.power_w);
#else
        printf("[ESP-NOW] RX Dash -> Remote | Speed: %.1f km/h | Batt: %.1fV | Power: %.0fW\n", 
                      current_telemetry.speed_kmh, current_telemetry.battery_voltage_v, current_telemetry.power_w);
#endif
#endif
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
            int bar_h = (int)(pct * 0.46f); // Map 100% to max 46px inner height (leaving padding)
            lv_obj_set_height(bar_board, bar_h);
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
            const char* sig = "[#][#][#][-]";
            const char* can = current_telemetry.can_alive ? "OK" : "!!";
            char stat_buf[64];
            snprintf(stat_buf, sizeof(stat_buf), "SIG: %s | CAN: %s", sig, can);
            lv_label_set_text(lbl_status, stat_buf);
            if (current_telemetry.can_alive) {
                lv_obj_set_style_text_color(lbl_status, lv_color_hex(0x00FF88), 0);
            } else {
                lv_obj_set_style_text_color(lbl_status, lv_color_hex(0xFF3300), 0);
            }
        }
    }
}

void RemoteApp::init() {
#ifdef ARDUINO
    // Setup Pins
    pinMode(PIN_POWER_ON, OUTPUT);
    digitalWrite(PIN_POWER_ON, HIGH); // Enable battery and screen power!
    
    pinMode(PIN_POT, INPUT);
    pinMode(PIN_BTN_UP, INPUT_PULLUP);
    pinMode(PIN_BTN_DOWN, INPUT_PULLUP);
    pinMode(PIN_BTN_LEFT, INPUT_PULLUP);
    pinMode(PIN_BTN_RIGHT, INPUT_PULLUP);
    pinMode(PIN_BTN_CONFIRM, INPUT_PULLUP);

    // Setup ESP-NOW
    preferences.begin("remote", false);
    if (digitalRead(PIN_BTN_CONFIRM) == LOW) {
        preferences.putInt("pot_min", 2048);
        preferences.putInt("pot_max", 2048);
        Serial.println("Calibration Reset!");
    }
    pot_min = preferences.getInt("pot_min", 2048);
    pot_max = preferences.getInt("pot_max", 2048);
    last_activity_time = millis();
    last_btn_state = 0;
    const uint8_t pins[] = {PIN_BTN_UP, PIN_BTN_DOWN, PIN_BTN_LEFT, PIN_BTN_RIGHT, PIN_BTN_CONFIRM};
    for (int i = 0; i < 5; i++) {
        if (digitalRead(pins[i]) == LOW) last_btn_state |= (1 << i);
    }

    WiFi.mode(WIFI_STA);
    if (esp_now_init() != ESP_OK) {
        Serial.println("Error initializing ESP-NOW");
        return;
    }

    esp_now_register_send_cb(onDataSent);
    esp_now_register_recv_cb(remote_onDataRecv);

    static esp_now_peer_info_t peerInfo;
    memcpy(peerInfo.peer_addr, receiver_mac, 6);
    peerInfo.channel = 0;  
    peerInfo.encrypt = false;
    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
        Serial.println("Failed to add Receiver peer");
        return;
    }

    static esp_now_peer_info_t dashPeerInfo;
    memcpy(dashPeerInfo.peer_addr, dash_mac, 6);
    dashPeerInfo.channel = 0;  
    dashPeerInfo.encrypt = false;
    if (esp_now_add_peer(&dashPeerInfo) != ESP_OK) {
        Serial.println("Failed to add Dash peer");
        return;
    }
#endif

    // Setup UI
#ifdef ARDUINO
    tft.init();
    tft.setRotation(0); // Symmetrical vertical orientation
    
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

    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x000000), 0);

    // 1. Connection Header Line
    lbl_status = lv_label_create(lv_scr_act());
    lv_obj_set_style_text_color(lbl_status, lv_color_hex(0x00FF88), 0);
    lv_obj_set_style_text_font(lbl_status, &lv_font_unscii_8, 0); // Sharp, tiny unscii 8
    lv_label_set_text(lbl_status, "SIG: [----] CAN: !!");
    lv_obj_align(lbl_status, LV_ALIGN_TOP_MID, 0, 6);

    // 2. Circular Speed Dial (Speed gauge)
    arc_speed = lv_arc_create(lv_scr_act());
    lv_obj_set_size(arc_speed, 110, 110);
    lv_arc_set_rotation(arc_speed, 135);
    lv_arc_set_bg_angles(arc_speed, 0, 270);
    lv_arc_set_value(arc_speed, 0);
    lv_obj_align(arc_speed, LV_ALIGN_TOP_MID, 0, 22);
    lv_obj_set_style_arc_color(arc_speed, lv_color_hex(0x222222), LV_PART_MAIN); // bg
    lv_obj_set_style_arc_color(arc_speed, lv_color_hex(0xC3B1E1), LV_PART_INDICATOR); // active purple
    lv_obj_remove_style(arc_speed, NULL, LV_PART_KNOB); // hide knob

    arc_throttle = lv_arc_create(lv_scr_act());
    lv_obj_set_size(arc_throttle, 110, 110);
    lv_arc_set_rotation(arc_throttle, 135);
    lv_arc_set_bg_angles(arc_throttle, 0, 270);
    lv_arc_set_value(arc_throttle, 0);
    lv_obj_align(arc_throttle, LV_ALIGN_TOP_MID, 0, 22);
    lv_obj_set_style_arc_color(arc_throttle, lv_color_hex(0x000000), LV_PART_MAIN); 
    lv_obj_set_style_arc_opa(arc_throttle, LV_OPA_0, LV_PART_MAIN); // invisible bg
    lv_obj_set_style_arc_color(arc_throttle, lv_color_hex(0xFF9900), LV_PART_INDICATOR); // Orange throttle
    lv_obj_set_style_arc_opa(arc_throttle, LV_OPA_50, LV_PART_INDICATOR); // 50% transparent
    lv_obj_remove_style(arc_throttle, NULL, LV_PART_KNOB);

    lbl_speed = lv_label_create(lv_scr_act());
    lv_obj_set_style_text_color(lbl_speed, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(lbl_speed, &lv_font_montserrat_36, 0); // Bold digits
    lv_label_set_text(lbl_speed, "0");
    lv_obj_align(lbl_speed, LV_ALIGN_TOP_MID, 0, 54);

    lv_obj_t* lbl_speed_unit = lv_label_create(lv_scr_act());
    lv_obj_set_style_text_color(lbl_speed_unit, lv_color_hex(0x555555), 0);
    lv_obj_set_style_text_font(lbl_speed_unit, &lv_font_unscii_16, 0);
    lv_label_set_text(lbl_speed_unit, "KM/H");
    lv_obj_align(lbl_speed_unit, LV_ALIGN_TOP_MID, 0, 92);

    // 3. Symmetrical Battery Columns (Board on left, Remote on right)
    // Board Battery Background Box
    lv_obj_t* bar_board_bg = lv_obj_create(lv_scr_act());
    lv_obj_set_size(bar_board_bg, 32, 50);
    lv_obj_align(bar_board_bg, LV_ALIGN_TOP_LEFT, 24, 150);
    lv_obj_set_style_bg_color(bar_board_bg, lv_color_hex(0x111111), 0);
    lv_obj_set_style_bg_opa(bar_board_bg, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(bar_board_bg, lv_color_hex(0x333333), 0);
    lv_obj_set_style_border_width(bar_board_bg, 1, 0);
    lv_obj_set_style_radius(bar_board_bg, 0, 0);
    lv_obj_clear_flag(bar_board_bg, LV_OBJ_FLAG_SCROLLABLE);

    // Board Battery Active Fill (child of bg)
    bar_board = lv_obj_create(bar_board_bg);
    lv_obj_set_size(bar_board, 30, 0); // Initial 0 height
    lv_obj_align(bar_board, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(bar_board, lv_color_hex(0x00FF88), 0); // Green
    lv_obj_set_style_bg_opa(bar_board, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(bar_board, 0, 0);
    lv_obj_set_style_radius(bar_board, 0, 0);
    lv_obj_clear_flag(bar_board, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* lbl_brd_title = lv_label_create(lv_scr_act());
    lv_obj_set_style_text_color(lbl_brd_title, lv_color_hex(0x555555), 0);
    lv_obj_set_style_text_font(lbl_brd_title, &lv_font_unscii_16, 0);
    lv_label_set_text(lbl_brd_title, "BRD"); // Short title fits beautifully
    lv_obj_align_to(lbl_brd_title, bar_board_bg, LV_ALIGN_OUT_BOTTOM_MID, 0, 4);

    lbl_board_volts = lv_label_create(lv_scr_act());
    lv_obj_set_style_text_color(lbl_board_volts, lv_color_hex(0x00FF88), 0);
    lv_obj_set_style_text_font(lbl_board_volts, &lv_font_unscii_16, 0);
    lv_label_set_text(lbl_board_volts, "0.0V");
    lv_obj_align_to(lbl_board_volts, lbl_brd_title, LV_ALIGN_OUT_BOTTOM_MID, 0, 2);

    // Remote Battery Background Box
    lv_obj_t* bar_remote_bg = lv_obj_create(lv_scr_act());
    lv_obj_set_size(bar_remote_bg, 32, 50);
    lv_obj_align(bar_remote_bg, LV_ALIGN_TOP_RIGHT, -24, 150);
    lv_obj_set_style_bg_color(bar_remote_bg, lv_color_hex(0x111111), 0);
    lv_obj_set_style_bg_opa(bar_remote_bg, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(bar_remote_bg, lv_color_hex(0x333333), 0);
    lv_obj_set_style_border_width(bar_remote_bg, 1, 0);
    lv_obj_set_style_radius(bar_remote_bg, 0, 0);
    lv_obj_clear_flag(bar_remote_bg, LV_OBJ_FLAG_SCROLLABLE);

    // Remote Battery Active Fill (child of bg)
    bar_remote = lv_obj_create(bar_remote_bg);
    lv_obj_set_size(bar_remote, 30, 0); // Initial 0 height
    lv_obj_align(bar_remote, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(bar_remote, lv_color_hex(0x00CCCC), 0); // Cyan
    lv_obj_set_style_bg_opa(bar_remote, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(bar_remote, 0, 0);
    lv_obj_set_style_radius(bar_remote, 0, 0);
    lv_obj_clear_flag(bar_remote, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* lbl_rem_title = lv_label_create(lv_scr_act());
    lv_obj_set_style_text_color(lbl_rem_title, lv_color_hex(0x555555), 0);
    lv_obj_set_style_text_font(lbl_rem_title, &lv_font_unscii_16, 0);
    lv_label_set_text(lbl_rem_title, "REM"); // Short title fits beautifully
    lv_obj_align_to(lbl_rem_title, bar_remote_bg, LV_ALIGN_OUT_BOTTOM_MID, 0, 4);

    lbl_remote_volts = lv_label_create(lv_scr_act());
    lv_obj_set_style_text_color(lbl_remote_volts, lv_color_hex(0x00CCCC), 0);
    lv_obj_set_style_text_font(lbl_remote_volts, &lv_font_unscii_16, 0);
    lv_label_set_text(lbl_remote_volts, "3.70V");
    lv_obj_align_to(lbl_remote_volts, lbl_rem_title, LV_ALIGN_OUT_BOTTOM_MID, 0, 2);

    // 4. Bottom Footer (Power Value)
    lbl_power = lv_label_create(lv_scr_act());
    lv_obj_set_style_text_color(lbl_power, lv_color_hex(0x00CCCC), 0);
    lv_obj_set_style_text_font(lbl_power, &lv_font_unscii_16, 0);
    lv_label_set_text(lbl_power, "POWER: 0W");
    lv_obj_align(lbl_power, LV_ALIGN_BOTTOM_MID, 0, -10);

#ifndef ARDUINO
    // Setup vertical slider on left margin for simulation potentiometer
    slider_pot = lv_slider_create(lv_scr_act());
    lv_slider_set_range(slider_pot, 0, 4095);
    lv_slider_set_value(slider_pot, 2048, LV_ANIM_OFF);
    lv_obj_set_size(slider_pot, 10, 80);
    lv_obj_align(slider_pot, LV_ALIGN_TOP_MID, 0, 160);
#endif

#ifdef ARDUINO
    Serial.println("Remote initialized.");
#endif
}

void RemoteApp::update() {
    static uint32_t last_send = 0;
    
    // Read Potentiometer & Remote Battery Volts
#ifdef ARDUINO
    int pot_val = analogRead(PIN_POT); // 0-4095
    // LilyGo T-Display S3 battery reading pin (or generic scaling for 3.7V - 4.2V range)
    float rem_volts = 3.7f + (analogRead(4) / 4095.0f) * 0.5f; 
#else
    if (slider_pot) sim_pot_val = lv_slider_get_value(slider_pot);
    int pot_val = sim_pot_val;
    // Simulate remote cell discharging slightly or varying based on throttle simulation
    float rem_volts = 3.92f - (pot_val / 4095.0f) * 0.12f;
#endif

    // Update Remote Battery Bar and Volts Text
    if (lbl_remote_volts) {
        char rem_buf[16];
        snprintf(rem_buf, sizeof(rem_buf), "%.2fV", rem_volts);
        lv_label_set_text(lbl_remote_volts, rem_buf);
    }
    if (bar_remote) {
        float pct = ((rem_volts - 3.7f) / 0.5f) * 100.0f;
        if (pct < 0) pct = 0; if (pct > 100) pct = 100;
        int bar_h = (int)(pct * 0.46f); // Map 100% to max 46px inner height (leaving padding)
        lv_obj_set_height(bar_remote, bar_h);
    }
    
    // Map to -100 to 100
    // Assume 2048 is neutral. Deadband around neutral.
#ifdef ARDUINO
    update_adc_calibration(pot_val);
    
    float p_max = (float)pot_max;
    float p_min = (float)pot_min;
    if (p_max < 2149.0f) p_max = 2149.0f; 
    if (p_min > 1947.0f) p_min = 1947.0f; 
#else
    float p_max = 4095.0f;
    float p_min = 0.0f;
#endif

    float throttle = 0.0f;
    if (pot_val > 2148) {
        throttle = ((pot_val - 2148.0f) / (p_max - 2148.0f)) * 100.0f;
    } else if (pot_val < 1948) {
        throttle = ((pot_val - 1948.0f) / (1948.0f - p_min)) * 100.0f; // Negative
    }
    
    if (throttle > 100.0f) throttle = 100.0f;
    if (throttle < -100.0f) throttle = -100.0f;

    if (arc_throttle) {
        lv_arc_set_value(arc_throttle, (int)abs(throttle));
    }

#ifdef ARDUINO
    check_inactivity_sleep(throttle);
#endif

    // Send every 50ms (20Hz)
    if (millis() - last_send > 50) {
        last_send = millis();
        ControlPacket pkt;
        pkt.throttle_percent = throttle;
#ifdef ARDUINO
        pkt.button_state = last_btn_state;
#else
        pkt.button_state = 0;
#endif
        esp_now_send(receiver_mac, (uint8_t *)&pkt, sizeof(ControlPacket));
        esp_now_send(dash_mac, (uint8_t *)&pkt, sizeof(ControlPacket));
        
#ifdef DEBUG_ESPNOW
#ifdef ARDUINO
        Serial.printf("[ESP-NOW] TX Remote -> Receiver | Throttle: %.1f%%\n", throttle);
#else
        // printf("[ESP-NOW] TX Remote -> Receiver | Throttle: %.1f%%\n", throttle);
#endif
#endif
    }
#ifdef ARDUINO
    lv_timer_handler();
#endif
}
