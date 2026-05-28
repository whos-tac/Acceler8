#include "settings_screen.h"
#include <lvgl.h>
#include <stdio.h>
#include <math.h>
#include "mechanical_config.h"
#include "odometer.h"
#include "ui_controller.h"

LV_FONT_DECLARE(lv_font_unscii_16);

namespace SettingsScreen {
    lv_obj_t* settings_screen;
    enum SettingMode {
        SETTING_MODE_MENU,
        SETTING_MODE_EDIT
    };
    SettingMode setting_mode = SETTING_MODE_MENU;
    int current_setting_idx = 0;
    lv_obj_t* setting_rows[5];
    lv_obj_t* setting_vals[4];

    // Colors copied from ui_controller
    lv_color_t color_cyan    = lv_color_hex(0x00CCCC);
    lv_color_t color_grey    = lv_color_hex(0x222222);
    lv_color_t color_white   = lv_color_hex(0xFFFFFF);
    lv_color_t color_dim     = lv_color_hex(0x555555);

    void update_settings_ui() {
        for(int i=0; i<5; i++) {
            if (i == current_setting_idx) {
                if (setting_mode == SETTING_MODE_EDIT) {
                    lv_obj_set_style_bg_color(setting_rows[i], lv_color_hex(0x550000), 0);
                    if (i < 4) lv_obj_set_style_text_color(setting_vals[i], lv_color_hex(0xFFFFFF), 0);
                } else {
                    lv_obj_set_style_bg_color(setting_rows[i], lv_color_hex(0x333333), 0);
                    if (i < 4) lv_obj_set_style_text_color(setting_vals[i], color_cyan, 0);
                }
            } else {
                lv_obj_set_style_bg_color(setting_rows[i], lv_color_hex(0x111111), 0);
                if (i < 4) lv_obj_set_style_text_color(setting_vals[i], color_dim, 0);
            }
        }

        char buf[32];
        snprintf(buf, sizeof(buf), "%d", motor_pole_pairs);
        lv_label_set_text(setting_vals[0], buf);

        snprintf(buf, sizeof(buf), "%.1f", gear_ratio);
        lv_label_set_text(setting_vals[1], buf);

        snprintf(buf, sizeof(buf), "%.0f", wheel_diameter_mm);
        lv_label_set_text(setting_vals[2], buf);

        snprintf(buf, sizeof(buf), "%.1f km", total_distance);
        if (setting_mode == SETTING_MODE_EDIT && current_setting_idx == 3) {
            lv_label_set_text(setting_vals[3], "PRESS CONFIRM TO RESET");
        } else {
            lv_label_set_text(setting_vals[3], buf);
        }
    }

    void init() {
        settings_screen = lv_obj_create(NULL);
        lv_obj_set_style_bg_color(settings_screen, lv_color_hex(0x000000), 0);

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
            "Save & Exit"
        };

        for (int i=0; i<5; i++) {
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

            if (i < 4) {
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
        update_settings_ui();
    }

    bool is_active() {
        return lv_scr_act() == settings_screen;
    }

    void update(uint8_t curr_btn) {
        static uint8_t last_btn = 0;
        
        bool pressed_up = (curr_btn & (1 << 0)) && !(last_btn & (1 << 0));
        bool pressed_down = (curr_btn & (1 << 1)) && !(last_btn & (1 << 1));
        bool pressed_left = (curr_btn & (1 << 2)) && !(last_btn & (1 << 2));
        bool pressed_right = (curr_btn & (1 << 3)) && !(last_btn & (1 << 3));
        bool pressed_confirm = (curr_btn & (1 << 4)) && !(last_btn & (1 << 4));
        
        last_btn = curr_btn;

        bool needs_update = false;

        if (setting_mode == SETTING_MODE_MENU) {
            if (pressed_up) { current_setting_idx = (current_setting_idx - 1 + 5) % 5; needs_update = true; }
            if (pressed_down) { current_setting_idx = (current_setting_idx + 1) % 5; needs_update = true; }
            if (pressed_confirm || pressed_right) {
                if (current_setting_idx == 4) {
                    // Save & Exit
                    save_mechanical_config();
                    UIController::show_main_screen();
                } else {
                    setting_mode = SETTING_MODE_EDIT;
                    needs_update = true;
                }
            }
            if (pressed_left) {
                UIController::show_main_screen();
            }
        } else {
            if (pressed_confirm || pressed_left) {
                setting_mode = SETTING_MODE_MENU;
                needs_update = true;
                if (current_setting_idx == 3 && pressed_confirm) { // Odometer reset
                    Odometer::reset();
                }
            }
            if (current_setting_idx == 0) { // Pole pairs
                if (pressed_up || pressed_right) { motor_pole_pairs++; needs_update = true; }
                if (pressed_down || pressed_left) { motor_pole_pairs--; needs_update = true; }
                if (motor_pole_pairs < 1) motor_pole_pairs = 1;
            } else if (current_setting_idx == 1) { // Gear ratio
                int gear_ratio_x10 = (int)(gear_ratio * 10.0f + 0.5f);
                if (pressed_up || pressed_right) { gear_ratio_x10 += 1; needs_update = true; }
                if (pressed_down || pressed_left) { gear_ratio_x10 -= 1; needs_update = true; }
                if (gear_ratio_x10 < 1) gear_ratio_x10 = 1;
                gear_ratio = gear_ratio_x10 / 10.0f;
            } else if (current_setting_idx == 2) { // Wheel dia
                if (pressed_up || pressed_right) { wheel_diameter_mm += 1.0f; needs_update = true; }
                if (pressed_down || pressed_left) { wheel_diameter_mm -= 1.0f; needs_update = true; }
                if (wheel_diameter_mm < 10.0f) wheel_diameter_mm = 10.0f;
            }
        }
        
        if (needs_update) {
            update_settings_ui();
        }
    }
}
