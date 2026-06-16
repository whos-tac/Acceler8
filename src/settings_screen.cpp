#include "settings_screen.h"
#include <lvgl.h>
#include <stdio.h>
#include <math.h>
#include "mechanical_config.h"
#include "odometer.h"
#include "ui_controller.h"
#include "espnow_dash.h"
#include "can_driver.h"

#ifdef ARDUINO
#include <Arduino.h>
#endif

LV_FONT_DECLARE(lv_font_unscii_16);

namespace SettingsScreen {
    lv_obj_t* settings_screen;
    enum SettingMode {
        SETTING_MODE_MENU,
        SETTING_MODE_EDIT
    };
    SettingMode setting_mode = SETTING_MODE_MENU;
    int current_setting_idx = 0;
    
    #define NUM_SETTINGS 10
    lv_obj_t* setting_rows[NUM_SETTINGS];
    lv_obj_t* setting_vals[NUM_SETTINGS - 1]; // Save & Exit has no val

    uint8_t esc_gear = 1;
    uint8_t esc_direction = 0;
    uint8_t display_brightness = 100;
    uint16_t underglow_hue = 0;
    bool headlight_active = false;

    // Continuous value floats for smooth scaling
    float f_brightness = 100.0f;
    float f_hue = 0.0f;

    // Colors copied from ui_controller
    lv_color_t color_cyan    = lv_color_hex(0x00CCCC);
    lv_color_t color_grey    = lv_color_hex(0x222222);
    lv_color_t color_white   = lv_color_hex(0xFFFFFF);
    lv_color_t color_dim     = lv_color_hex(0x555555);

    void update_settings_ui() {
        for(int i=0; i<NUM_SETTINGS; i++) {
            if (i == current_setting_idx) {
                if (setting_mode == SETTING_MODE_EDIT && i < (NUM_SETTINGS - 1)) {
                    lv_obj_set_style_bg_color(setting_rows[i], lv_color_hex(0x550000), 0);
                    lv_obj_set_style_text_color(setting_vals[i], color_white, 0);
                } else {
                    lv_obj_set_style_bg_color(setting_rows[i], lv_color_hex(0x333333), 0);
                    if (i < (NUM_SETTINGS - 1)) lv_obj_set_style_text_color(setting_vals[i], color_cyan, 0);
                }
            } else {
                lv_obj_set_style_bg_color(setting_rows[i], lv_color_hex(0x111111), 0);
                if (i < (NUM_SETTINGS - 1)) lv_obj_set_style_text_color(setting_vals[i], color_dim, 0);
            }
        }

        char buf[32];
        
        // 0: Pole Pairs
        snprintf(buf, sizeof(buf), "%d", motor_pole_pairs);
        lv_label_set_text(setting_vals[0], buf);

        // 1: Gear Ratio
        snprintf(buf, sizeof(buf), "%.1f", gear_ratio);
        lv_label_set_text(setting_vals[1], buf);

        // 2: Wheel Dia
        snprintf(buf, sizeof(buf), "%.0f", wheel_diameter_mm);
        lv_label_set_text(setting_vals[2], buf);

        // 3: Odometer
        snprintf(buf, sizeof(buf), "%.1f km", total_distance);
        if (setting_mode == SETTING_MODE_EDIT && current_setting_idx == 3) {
            lv_label_set_text(setting_vals[3], "PRESS CONFIRM");
        } else {
            lv_label_set_text(setting_vals[3], buf);
        }
        
        // 4: ESC Gear
        const char* gear_strs[] = {"None", "Low", "Medium", "High"};
        lv_label_set_text(setting_vals[4], gear_strs[esc_gear % 4]);
        
        // 5: ESC Direction
        const char* dir_strs[] = {"Forward", "Reverse"};
        lv_label_set_text(setting_vals[5], dir_strs[esc_direction % 2]);

        // 6: Headlight
        lv_label_set_text(setting_vals[6], headlight_active ? "On" : "Off");
        
        // 7: Brightness
        snprintf(buf, sizeof(buf), "%d%%", display_brightness);
        lv_label_set_text(setting_vals[7], buf);
        
        // 8: Underglow Hue
        snprintf(buf, sizeof(buf), "%d deg", underglow_hue);
        lv_label_set_text(setting_vals[8], buf);

        // Scroll into view
        lv_obj_scroll_to_view(setting_rows[current_setting_idx], LV_ANIM_ON);
    }

    void init() {
        settings_screen = lv_obj_create(NULL);
        lv_obj_set_style_bg_color(settings_screen, lv_color_hex(0x000000), 0);
        lv_obj_add_flag(settings_screen, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t* title = lv_label_create(settings_screen);
        lv_obj_set_style_text_color(title, color_cyan, 0);
        lv_obj_set_style_text_font(title, &lv_font_unscii_16, 0);
        lv_label_set_text(title, "SETTINGS");
        lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);

        const char* labels[] = {
            "Pole Pairs",
            "Gear Ratio",
            "Wheel Dia (mm)",
            "Odometer",
            "ESC Gear",
            "ESC Direction",
            "Headlight",
            "Brightness",
            "Underglow Hue",
            "Save & Exit"
        };

        for (int i=0; i<NUM_SETTINGS; i++) {
            setting_rows[i] = lv_obj_create(settings_screen);
            lv_obj_set_size(setting_rows[i], 400, 60);
            lv_obj_align(setting_rows[i], LV_ALIGN_TOP_MID, 0, 70 + i*70);
            lv_obj_set_style_bg_color(setting_rows[i], color_grey, 0);
            lv_obj_set_style_border_width(setting_rows[i], 0, 0);
            lv_obj_set_style_radius(setting_rows[i], 10, 0);
            lv_obj_clear_flag(setting_rows[i], LV_OBJ_FLAG_SCROLLABLE);

            lv_obj_t* lbl = lv_label_create(setting_rows[i]);
            lv_obj_set_style_text_color(lbl, color_white, 0);
            lv_obj_set_style_text_font(lbl, &lv_font_unscii_16, 0);
            lv_label_set_text(lbl, labels[i]);
            lv_obj_align(lbl, LV_ALIGN_LEFT_MID, 20, 0);

            if (i < (NUM_SETTINGS - 1)) {
                setting_vals[i] = lv_label_create(setting_rows[i]);
                lv_obj_set_style_text_color(setting_vals[i], color_cyan, 0);
                lv_obj_set_style_text_font(setting_vals[i], &lv_font_unscii_16, 0);
                lv_label_set_text(setting_vals[i], "-");
                lv_obj_align(setting_vals[i], LV_ALIGN_RIGHT_MID, -20, 0);
            }
        }
    }

    void enter() {
        lv_scr_load(settings_screen);
        current_setting_idx = 0;
        setting_mode = SETTING_MODE_MENU;
        f_brightness = (float)display_brightness;
        f_hue = (float)underglow_hue;
        update_settings_ui();
    }

    bool is_active() {
        return lv_scr_act() == settings_screen;
    }

    void update(uint8_t curr_btn) {
        bool pressed_up = (curr_btn & (1 << 0)) && !(UIController::global_last_btn & (1 << 0));
        bool pressed_down = (curr_btn & (1 << 1)) && !(UIController::global_last_btn & (1 << 1));
        bool pressed_left = (curr_btn & (1 << 2)) && !(UIController::global_last_btn & (1 << 2));
        bool pressed_right = (curr_btn & (1 << 3)) && !(UIController::global_last_btn & (1 << 3));
        bool pressed_confirm = (curr_btn & (1 << 4)) && !(UIController::global_last_btn & (1 << 4));
        
        UIController::global_last_btn = curr_btn;

        bool needs_update = false;
        
        // Continuous throttle handling for active editing
        static uint32_t last_tick = 0;
#ifdef ARDUINO
        uint32_t now = millis();
#else
        uint32_t now = ::millis();
#endif
        if (last_tick == 0) last_tick = now;
        float dt = (now - last_tick) / 1000.0f;
        last_tick = now;

        if (setting_mode == SETTING_MODE_EDIT) {
            float throttle = g_vehicle_state.remote_throttle; // -100 to 100
            
            // Apply deadzone to throttle for UI (to avoid drift)
            if (fabs(throttle) < 5.0f) throttle = 0.0f;
            
            if (current_setting_idx == 7) { // Brightness (10 to 100)
                if (throttle != 0.0f) {
                    f_brightness += (throttle / 100.0f) * 100.0f * dt; // 100% per sec at full throttle
                    if (f_brightness < 10.0f) f_brightness = 10.0f;
                    if (f_brightness > 100.0f) f_brightness = 100.0f;
                    display_brightness = (uint8_t)f_brightness;
                    needs_update = true;
                }
            } else if (current_setting_idx == 8) { // Hue (0 to 359)
                if (throttle != 0.0f) {
                    f_hue += (throttle / 100.0f) * 360.0f * dt; // 360 deg per sec at full throttle
                    while (f_hue >= 360.0f) f_hue -= 360.0f;
                    while (f_hue < 0.0f) f_hue += 360.0f;
                    underglow_hue = (uint16_t)f_hue;
                    needs_update = true;
                }
            }
        }

        if (setting_mode == SETTING_MODE_MENU) {
            if (pressed_up) { current_setting_idx = (current_setting_idx - 1 + NUM_SETTINGS) % NUM_SETTINGS; needs_update = true; }
            if (pressed_down) { current_setting_idx = (current_setting_idx + 1) % NUM_SETTINGS; needs_update = true; }
            if (pressed_confirm) {
                if (current_setting_idx == 9) { // Save & Exit
                    save_mechanical_config();
                    UIController::show_main_screen();
                } else {
                    setting_mode = SETTING_MODE_EDIT;
                    needs_update = true;
                }
            }
        } else {
            // EDIT MODE Discrete inputs
            if (pressed_confirm) {
                setting_mode = SETTING_MODE_MENU;
                needs_update = true;
                if (current_setting_idx == 3) { // Odometer reset
                    Odometer::reset();
                }
            } else {
                if (current_setting_idx == 0) { // Pole pairs
                    if (pressed_right) { motor_pole_pairs++; needs_update = true; }
                    if (pressed_left) { motor_pole_pairs--; needs_update = true; }
                    if (motor_pole_pairs < 1) motor_pole_pairs = 1;
                } else if (current_setting_idx == 1) { // Gear ratio
                    int gear_ratio_x10 = (int)(gear_ratio * 10.0f + 0.5f);
                    if (pressed_right) { gear_ratio_x10 += 1; needs_update = true; }
                    if (pressed_left) { gear_ratio_x10 -= 1; needs_update = true; }
                    if (gear_ratio_x10 < 1) gear_ratio_x10 = 1;
                    gear_ratio = gear_ratio_x10 / 10.0f;
                } else if (current_setting_idx == 2) { // Wheel dia
                    if (pressed_right) { wheel_diameter_mm += 1.0f; needs_update = true; }
                    if (pressed_left) { wheel_diameter_mm -= 1.0f; needs_update = true; }
                    if (wheel_diameter_mm < 10.0f) wheel_diameter_mm = 10.0f;
                } else if (current_setting_idx == 4) { // ESC Gear
                    if (pressed_right) { esc_gear = (esc_gear + 1) % 4; needs_update = true; }
                    if (pressed_left) { esc_gear = (esc_gear - 1 + 4) % 4; needs_update = true; }
                } else if (current_setting_idx == 5) { // ESC Direction
                    if (pressed_right) { esc_direction = (esc_direction + 1) % 2; needs_update = true; }
                    if (pressed_left) { esc_direction = (esc_direction - 1 + 2) % 2; needs_update = true; }
                } else if (current_setting_idx == 6) { // Headlight
                    if (pressed_right || pressed_left) { headlight_active = !headlight_active; needs_update = true; }
                }
            }
        }
        
        if (needs_update) {
            update_settings_ui();
        }
    }
}
