#include "ui_controller.h"
#include <lvgl.h>
#include "can_driver.h"
#include "mechanical_config.h"
#include <stdio.h>
#include <math.h>
#include "odometer.h"
#include "settings_screen.h"

#ifdef ARDUINO
#include <Arduino.h>
#else
extern "C" uint32_t millis();
#endif

LV_FONT_DECLARE(lv_font_block_300);
LV_FONT_DECLARE(lv_font_block_72);
LV_FONT_DECLARE(lv_font_block_56);
LV_FONT_DECLARE(lv_font_block_24);

namespace UIController {

    // ================================================================
    // GLOBALS
    // ================================================================
    lv_obj_t * label_speed_val;
    lv_obj_t * label_pwr_val;
    lv_obj_t * label_temp_esc_val;
    lv_obj_t * label_trip_val;
    lv_obj_t * label_range_val;
    lv_obj_t * label_whkm_val;

    lv_obj_t * batt_bars[10];
    lv_color_t bar_colors[10];
    lv_obj_t * label_footer;
    lv_obj_t * label_can_status;
    
    lv_obj_t * alert_overlay;
    lv_obj_t * alert_label;

    // ================================================================
    // Colors
    // ================================================================
    lv_color_t color_purple  = lv_color_hex(0xC3B1E1);
    lv_color_t color_white   = lv_color_hex(0xFFFFFF);
    lv_color_t color_grey    = lv_color_hex(0x222222);
    lv_color_t color_accent  = lv_color_hex(0xFF3300);
    lv_color_t color_green   = lv_color_hex(0x00FF88);
    lv_color_t color_cyan    = lv_color_hex(0x00CCCC);
    lv_color_t color_dim     = lv_color_hex(0x555555);

    // ================================================================
    // Layout geometry (480x480)
    //
    // ┌────────────────────────┬──────────────┐
    // │                        │  TRIP        │
    // │         SPEED          │  RANGE       │
    // │         (HUGE)         │  Wh/km       │
    // │                        │  ESC Temp    │
    // ├────────────────────────┤              │
    // │   WATT                 │              │
    // ├──────────────────────────────────────┤
    // │ ▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓  BATTERY STRIP   │
    // ├──────────────────────────────────────┤
    // │ DASH v2.0                    CAN OK  │
    // └────────────────────────────────────-─┘
    // ================================================================

    static const int MARGIN       = 8;
    static const int SCREEN_W     = 480;
    static const int SCREEN_H     = 480;
    static const int FOOTER_H     = 20;
    static const int FOOTER_Y     = SCREEN_H - FOOTER_H;           // 460

    // Battery bar — sits above footer
    static const int BAR_H        = 44;
    static const int BAR_GAP_BTM  = 4;
    static const int BAR_Y        = FOOTER_Y - BAR_GAP_BTM - BAR_H; // 412
    static const int BAR_COUNT    = 10;
    static const int BAR_SEG_GAP  = 6;
    static const int BAR_W        = (SCREEN_W - 2*MARGIN - (BAR_COUNT-1)*BAR_SEG_GAP) / BAR_COUNT;

    // Usable content area: y=[MARGIN … BAR_Y-MARGIN]
    static const int CONTENT_TOP  = MARGIN;                        // 8
    static const int CONTENT_BOT  = BAR_Y - MARGIN;               // 404
    static const int CONTENT_H    = CONTENT_BOT - CONTENT_TOP;    // 396

    // Right column: 4 equally-tall stat boxes filling full content height
    static const int STAT_W       = 160;
    static const int STAT_X       = SCREEN_W - MARGIN - STAT_W;   // 312
    static const int STAT_GAP     = 8;
    static const int STAT_BOX_H   = (CONTENT_H - 3 * STAT_GAP) / 4; // ~87

    // Left column: Speed on top, WATT box at the bottom
    static const int LEFT_W       = STAT_X - MARGIN - 4;          // 300
    static const int WATT_H       = 90;  // block_72 (72px) + ~18px for title
    static const int WATT_Y       = CONTENT_BOT - WATT_H;
    static const int WATT_X       = MARGIN;
    // Speed label: from CONTENT_TOP to 4px above WATT box
    static const int SPEED_X      = MARGIN;
    static const int SPEED_Y      = CONTENT_TOP;                   // 8

    // ================================================================
    // Widget Builders
    // ================================================================

    void create_stat_box(int x, int y, int w, int h,
                         const char* title, lv_obj_t** val_label, lv_color_t color) {
        lv_obj_t* bg = lv_obj_create(lv_scr_act());
        lv_obj_set_size(bg, w, h);
        lv_obj_align(bg, LV_ALIGN_TOP_LEFT, x, y);
        lv_obj_set_style_bg_color(bg, lv_color_hex(0x000000), 0);
        lv_obj_set_style_border_width(bg, 1, 0);
        lv_obj_set_style_border_color(bg, color_dim, 0);
        lv_obj_set_style_radius(bg, 0, 0);
        // Remove default LVGL padding so labels truly hug corners
        lv_obj_set_style_pad_all(bg, 0, 0);
        lv_obj_clear_flag(bg, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t* lbl_title = lv_label_create(bg);
        lv_obj_set_style_text_color(lbl_title, color_dim, 0);
        lv_obj_set_style_text_font(lbl_title, &lv_font_unscii_16, 0);
        lv_label_set_text(lbl_title, title);
        lv_obj_align(lbl_title, LV_ALIGN_TOP_LEFT, 3, 2);  // tight top-left corner

        *val_label = lv_label_create(bg);
        lv_obj_set_style_text_color(*val_label, color, 0);
        lv_obj_set_style_text_font(*val_label, &lv_font_block_56, 0);
        lv_label_set_text(*val_label, "0");
        lv_obj_align(*val_label, LV_ALIGN_BOTTOM_RIGHT, 0, 0); // tight bottom-right corner
    }

    void create_hero_box(int x, int y, int w, int h,
                         const char* title, lv_obj_t** val_label,
                         lv_color_t border_color, lv_color_t val_color) {
        lv_obj_t* panel = lv_obj_create(lv_scr_act());
        lv_obj_set_size(panel, w, h);
        lv_obj_align(panel, LV_ALIGN_TOP_LEFT, x, y);
        lv_obj_set_style_bg_color(panel, lv_color_hex(0x000000), 0);
        lv_obj_set_style_border_width(panel, 2, 0);
        lv_obj_set_style_border_color(panel, border_color, 0);
        lv_obj_set_style_radius(panel, 0, 0);
        // Remove default LVGL padding so labels truly hug corners
        lv_obj_set_style_pad_all(panel, 0, 0);
        lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t* lbl_t = lv_label_create(panel);
        lv_obj_set_style_text_color(lbl_t, border_color, 0);
        lv_obj_set_style_text_font(lbl_t, &lv_font_unscii_16, 0);
        lv_label_set_text(lbl_t, title);
        lv_obj_align(lbl_t, LV_ALIGN_TOP_LEFT, 3, 2);  // tight top-left corner

        *val_label = lv_label_create(panel);
        lv_obj_set_style_text_color(*val_label, val_color, 0);
        lv_obj_set_style_text_font(*val_label, &lv_font_block_72, 0);
        lv_label_set_text(*val_label, "0");
        // block_72 at 90px box: fills from y≈18 (below title) to y=90 → top to bottom
        lv_obj_align(*val_label, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
    }

    // ================================================================
    // INIT
    // ================================================================
    bool debug_mode_active = false;
    lv_obj_t* debug_screen;
    lv_obj_t* main_screen;
    lv_obj_t* debug_label;
    int tap_count = 0;
    uint32_t last_tap_time = 0;

    static void top_right_event_cb(lv_event_t * e) {
        uint32_t t = lv_tick_get();
        if (t - last_tap_time > 1000) {
            tap_count = 0;
        }
        tap_count++;
        last_tap_time = t;
        if (tap_count >= 3) {
            debug_mode_active = !debug_mode_active;
            if (debug_mode_active) {
                lv_scr_load(debug_screen);
            } else {
                show_main_screen();
            }
            tap_count = 0;
        }
    }

    void show_main_screen() {
        lv_scr_load(main_screen);
    }

    void init() {
        main_screen = lv_scr_act();
        lv_obj_set_style_bg_color(main_screen, lv_color_hex(0x000000), 0);

        // Top right invisible button for debug
        lv_obj_t* btn_debug = lv_btn_create(main_screen);
        lv_obj_set_size(btn_debug, 120, 120);
        lv_obj_align(btn_debug, LV_ALIGN_TOP_RIGHT, 0, 0);
        lv_obj_set_style_bg_opa(btn_debug, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(btn_debug, 0, 0);
        lv_obj_set_style_shadow_width(btn_debug, 0, 0);
        lv_obj_add_event_cb(btn_debug, top_right_event_cb, LV_EVENT_CLICKED, NULL);

        debug_screen = lv_obj_create(NULL);
        lv_obj_set_style_bg_color(debug_screen, lv_color_hex(0x000088), 0);
        debug_label = lv_label_create(debug_screen);
        lv_obj_set_style_text_color(debug_label, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_text_font(debug_label, &lv_font_unscii_16, 0);
        lv_label_set_text(debug_label, "DEBUG MODE");
        lv_obj_align(debug_label, LV_ALIGN_TOP_LEFT, 10, 10);
        
        lv_obj_t* btn_debug_back = lv_btn_create(debug_screen);
        lv_obj_set_size(btn_debug_back, 120, 120);
        lv_obj_align(btn_debug_back, LV_ALIGN_TOP_RIGHT, 0, 0);
        lv_obj_set_style_bg_opa(btn_debug_back, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(btn_debug_back, 0, 0);
        lv_obj_set_style_shadow_width(btn_debug_back, 0, 0);
        lv_obj_add_event_cb(btn_debug_back, top_right_event_cb, LV_EVENT_CLICKED, NULL);

        // ── SPEED: top-left, fills available height above WATT ──────
        // Use a fixed-size container to control layout precisely
        lv_obj_t* speed_container = lv_obj_create(lv_scr_act());
        lv_obj_set_size(speed_container, LEFT_W, WATT_Y - CONTENT_TOP - 4);
        lv_obj_align(speed_container, LV_ALIGN_TOP_LEFT, SPEED_X, SPEED_Y);
        lv_obj_set_style_bg_opa(speed_container, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(speed_container, 0, 0);
        lv_obj_set_style_pad_all(speed_container, 0, 0);
        lv_obj_clear_flag(speed_container, LV_OBJ_FLAG_SCROLLABLE);

        label_speed_val = lv_label_create(speed_container);
        lv_obj_set_style_text_color(label_speed_val, color_purple, 0);
        lv_obj_set_style_text_font(label_speed_val, &lv_font_block_300, 0);
        lv_label_set_text(label_speed_val, "0");
        lv_label_set_long_mode(label_speed_val, LV_LABEL_LONG_CLIP);
        lv_obj_set_width(label_speed_val, LEFT_W);
        // Anchor to bottom of its container so it "nearly touches" the WATT box
        lv_obj_align(label_speed_val, LV_ALIGN_BOTTOM_LEFT, 0, 0);

        // ── WATT: bottom of left column ─────────────────────────────
        create_hero_box(WATT_X, WATT_Y, LEFT_W, WATT_H,
                        "WATT", &label_pwr_val, color_cyan, color_cyan);

        // ── RIGHT COLUMN: 4 equally-spaced boxes, full content height ─
        int y = CONTENT_TOP;
        create_stat_box(STAT_X, y, STAT_W, STAT_BOX_H, "TRIP",     &label_trip_val,     color_white);
        y += STAT_BOX_H + STAT_GAP;
        create_stat_box(STAT_X, y, STAT_W, STAT_BOX_H, "RANGE",    &label_range_val,    color_cyan);
        y += STAT_BOX_H + STAT_GAP;
        create_stat_box(STAT_X, y, STAT_W, STAT_BOX_H, "Wh/km",   &label_whkm_val,     color_green);
        y += STAT_BOX_H + STAT_GAP;
        create_stat_box(STAT_X, y, STAT_W, STAT_BOX_H, "ESC Temp", &label_temp_esc_val, color_purple);

        // ── BATTERY BAR ──────────────────────────────────────────────
        bar_colors[0] = lv_color_hex(0xFF0000); bar_colors[1] = lv_color_hex(0xFF4400);
        bar_colors[2] = lv_color_hex(0xFF8800); bar_colors[3] = lv_color_hex(0xFFCC00);
        bar_colors[4] = lv_color_hex(0xFFFF00); bar_colors[5] = lv_color_hex(0xCCFF00);
        bar_colors[6] = lv_color_hex(0x88FF00); bar_colors[7] = lv_color_hex(0x44FF00);
        bar_colors[8] = lv_color_hex(0x00FF00); bar_colors[9] = lv_color_hex(0x00FFFF);

        for (int i = 0; i < BAR_COUNT; i++) {
            batt_bars[i] = lv_obj_create(lv_scr_act());
            lv_obj_set_size(batt_bars[i], BAR_W, BAR_H);
            lv_obj_align(batt_bars[i], LV_ALIGN_TOP_LEFT,
                         MARGIN + i * (BAR_W + BAR_SEG_GAP), BAR_Y);
            lv_obj_set_style_radius(batt_bars[i], 0, 0);
            lv_obj_set_style_border_width(batt_bars[i], 2, 0);
            lv_obj_set_style_border_color(batt_bars[i], color_grey, 0);
            lv_obj_set_style_bg_opa(batt_bars[i], LV_OPA_TRANSP, 0);
            lv_obj_clear_flag(batt_bars[i], LV_OBJ_FLAG_SCROLLABLE);
        }

        // ── FOOTER ───────────────────────────────────────────────────
        label_footer = lv_label_create(lv_scr_act());
        lv_obj_set_style_text_color(label_footer, color_dim, 0);
        lv_obj_set_style_text_font(label_footer, &lv_font_unscii_16, 0);
        lv_label_set_text(label_footer, "DASH v2.2");
        lv_obj_align(label_footer, LV_ALIGN_TOP_LEFT, MARGIN, FOOTER_Y + 2);

        label_can_status = lv_label_create(lv_scr_act());
        lv_obj_set_style_text_font(label_can_status, &lv_font_unscii_16, 0);
        lv_obj_set_style_text_color(label_can_status, color_green, 0);
        lv_label_set_text(label_can_status, "CAN OK");
        lv_obj_align(label_can_status, LV_ALIGN_TOP_RIGHT, -MARGIN, FOOTER_Y + 2);

        // ── ALERT OVERLAY ────────────────────────────────────────────
        alert_overlay = lv_obj_create(main_screen);
        lv_obj_set_size(alert_overlay, SCREEN_W, SCREEN_H);
        lv_obj_align(alert_overlay, LV_ALIGN_CENTER, 0, 0);
        lv_obj_set_style_bg_color(alert_overlay, lv_color_hex(0xFF0000), 0);
        lv_obj_set_style_bg_opa(alert_overlay, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(alert_overlay, 0, 0);
        lv_obj_set_style_radius(alert_overlay, 0, 0);
        lv_obj_clear_flag(alert_overlay, LV_OBJ_FLAG_SCROLLABLE);

        alert_label = lv_label_create(alert_overlay);
        lv_obj_set_style_text_color(alert_label, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_text_font(alert_label, &lv_font_block_56, 0);
        lv_label_set_text(alert_label, "ALERT");
        lv_obj_align(alert_label, LV_ALIGN_CENTER, 0, 0);
        
        lv_obj_add_flag(alert_overlay, LV_OBJ_FLAG_HIDDEN);

        SettingsScreen::init();
    }

    // ================================================================
    // UPDATE
    // ================================================================
    void update() {
        uint8_t curr_btn = g_vehicle_state.remote_button_state;

        if (SettingsScreen::is_active()) {
            SettingsScreen::update(curr_btn);
            return;
        }

        static uint8_t last_btn = 0;
        bool pressed_confirm = (curr_btn & (1 << 4)) && !(last_btn & (1 << 4));
        last_btn = curr_btn;

        bool alert_hidden = lv_obj_has_flag(alert_overlay, LV_OBJ_FLAG_HIDDEN);

        if (pressed_confirm && !debug_mode_active && lv_scr_act() == main_screen && alert_hidden) {
            SettingsScreen::enter();
            return;
        }

        int32_t erpm = g_vehicle_state.erpm;
        float v      = g_vehicle_state.battery_voltage_v;
        int32_t tach = g_vehicle_state.tachometer;
        float t_esc  = g_vehicle_state.mosfet_temp_c;
        float range  = g_vehicle_state.range_km;
        float whkm   = g_vehicle_state.wh_per_km;
        bool can_ok  = g_vehicle_state.can_alive;
        float pwr    = g_vehicle_state.power_w;
        float speed  = calculate_speed_kmh(erpm);

        if (debug_mode_active) {
            char debug_buf[128];
            snprintf(debug_buf, sizeof(debug_buf), "DEBUG MODE\nSpeed: %.1f\nBatt: %.1f\nPwr: %.0f\nCAN: %d", speed, v, pwr, can_ok);
            lv_label_set_text(debug_label, debug_buf);
            return;
        }

        char buf[16];

        // SPEED
        snprintf(buf, sizeof(buf), "%02.0f", speed);
        lv_label_set_text(label_speed_val, buf);

        // WATT
        snprintf(buf, sizeof(buf), "%.0f", pwr);
        lv_label_set_text(label_pwr_val, buf);
        if (pwr < 0) lv_obj_set_style_text_color(label_pwr_val, color_green, 0);
        else         lv_obj_set_style_text_color(label_pwr_val, color_cyan, 0);

        // TRIP
        float trip_km = tach / 100000.0f;
        snprintf(buf, sizeof(buf), "%.1f", trip_km);
        lv_label_set_text(label_trip_val, buf);

        // RANGE
        if (range < 10.0f) snprintf(buf, sizeof(buf), "%.1f", range);
        else               snprintf(buf, sizeof(buf), "%.0f", range);
        lv_label_set_text(label_range_val, buf);

        // WH/KM
        snprintf(buf, sizeof(buf), "%.1f", whkm);
        lv_label_set_text(label_whkm_val, buf);

        // ESC TEMP
        snprintf(buf, sizeof(buf), "%.0f C", t_esc);
        lv_label_set_text(label_temp_esc_val, buf);
        if (t_esc > 70)
            lv_obj_set_style_text_color(label_temp_esc_val, color_accent, 0);
        else
            lv_obj_set_style_text_color(label_temp_esc_val, color_purple, 0);

        // BATTERY STRIP
        float pct = ((v - 32.0f) / 10.0f) * 100.0f;
        if (pct < 0) pct = 0;
        if (pct > 100) pct = 100;
        int active_bars = (int)(pct / 10.0f);
        for (int i = 0; i < BAR_COUNT; i++) {
            if (i < active_bars) {
                lv_obj_set_style_bg_color(batt_bars[i], bar_colors[i], 0);
                lv_obj_set_style_bg_opa(batt_bars[i], LV_OPA_COVER, 0);
            } else {
                lv_obj_set_style_bg_opa(batt_bars[i], LV_OPA_TRANSP, 0);
            }
        }

        // CAN STATUS
        if (can_ok) {
            lv_label_set_text(label_can_status, "CAN OK");
            lv_obj_set_style_text_color(label_can_status, color_green, 0);
        } else {
            lv_label_set_text(label_can_status, "CAN !!");
            lv_obj_set_style_text_color(label_can_status, color_accent, 0);
        }

        // ── GLOBAL ALERTS ────────────────────────────────────────────
        bool can_timeout = (!g_vehicle_state.can_alive && g_vehicle_state.has_received_can);
        bool overtemp = (t_esc > 85.0f || g_vehicle_state.motor_temp_c > 100.0f);
        bool remote_dc = g_vehicle_state.remote_disconnected;
        
        if (can_timeout || overtemp || remote_dc) {
            lv_obj_clear_flag(alert_overlay, LV_OBJ_FLAG_HIDDEN);
            if (can_timeout) {
                lv_label_set_text(alert_label, "ERR: CAN TIMEOUT");
            } else if (remote_dc) {
                lv_label_set_text(alert_label, "REMOTE DISCONNECT");
            } else if (overtemp) {
                lv_label_set_text(alert_label, "ERR: OVERTEMP");
            }
        } else {
            lv_obj_add_flag(alert_overlay, LV_OBJ_FLAG_HIDDEN);
        }
    }
}
