#include "ui_controller.h"
#include <lvgl.h>
#include "can_driver.h"

namespace UIController {

    lv_obj_t * arc_erpm;
    lv_obj_t * label_erpm;
    lv_obj_t * label_battery;
    lv_obj_t * bar_battery;
    lv_obj_t * label_current;
    lv_obj_t * label_temp;

    void init() {
        // --- Futuristic Purple and Dark Grey-Blue Theme ---
        lv_color_t color_bg = lv_color_hex(0x05050A); // Deep black background
        lv_color_t color_purple = lv_color_hex(0xA020F0); // Neon purple
        lv_color_t color_darkblue = lv_color_hex(0x1a2332); // Dark grey-blue panels
        lv_color_t color_white = lv_color_hex(0xFFFFFF); // Text

        // Background
        lv_obj_set_style_bg_color(lv_scr_act(), color_bg, 0);
        
        // --- CENTER GAUGE (ERPM) ---
        arc_erpm = lv_arc_create(lv_scr_act());
        lv_obj_set_size(arc_erpm, 320, 320);
        lv_arc_set_rotation(arc_erpm, 135);
        lv_arc_set_bg_angles(arc_erpm, 0, 270);
        lv_arc_set_range(arc_erpm, 0, 50000);
        lv_obj_center(arc_erpm);
        
        // Glassmorphism/Neon Arc Styling
        lv_obj_set_style_arc_color(arc_erpm, color_darkblue, LV_PART_MAIN);
        lv_obj_set_style_arc_color(arc_erpm, color_purple, LV_PART_INDICATOR);
        lv_obj_set_style_arc_width(arc_erpm, 16, LV_PART_MAIN);
        lv_obj_set_style_arc_width(arc_erpm, 16, LV_PART_INDICATOR);
        lv_obj_set_style_opa(arc_erpm, 0, LV_PART_KNOB); // Hide invisible thumb knob

        label_erpm = lv_label_create(lv_scr_act());
        lv_obj_set_style_text_color(label_erpm, color_white, 0);
        lv_obj_align(label_erpm, LV_ALIGN_CENTER, 0, 0);
        lv_label_set_text(label_erpm, "0\nERPM");
        lv_obj_set_style_text_align(label_erpm, LV_TEXT_ALIGN_CENTER, 0);

        // --- TOP (Battery Panel) ---
        lv_obj_t* panel_batt = lv_obj_create(lv_scr_act());
        lv_obj_set_size(panel_batt, 240, 60);
        lv_obj_align(panel_batt, LV_ALIGN_TOP_MID, 0, 20);
        lv_obj_set_style_bg_color(panel_batt, color_darkblue, 0);
        lv_obj_set_style_border_width(panel_batt, 0, 0);
        lv_obj_set_style_radius(panel_batt, 16, 0); // Rounded glass panels

        label_battery = lv_label_create(panel_batt);
        lv_obj_set_style_text_color(label_battery, color_white, 0);
        lv_obj_align(label_battery, LV_ALIGN_LEFT_MID, 10, 0);
        lv_label_set_text(label_battery, "42.0V");

        bar_battery = lv_bar_create(panel_batt);
        lv_obj_set_size(bar_battery, 120, 16);
        lv_obj_align(bar_battery, LV_ALIGN_RIGHT_MID, -10, 0);
        lv_obj_set_style_bg_color(bar_battery, lv_color_hex(0x000000), LV_PART_MAIN);
        lv_obj_set_style_bg_color(bar_battery, color_purple, LV_PART_INDICATOR);
        lv_bar_set_range(bar_battery, 0, 100);

        // --- BOTTOM LEFT (Current Draw) ---
        lv_obj_t* panel_current = lv_obj_create(lv_scr_act());
        lv_obj_set_size(panel_current, 140, 60);
        lv_obj_align(panel_current, LV_ALIGN_BOTTOM_LEFT, 20, -20);
        lv_obj_set_style_bg_color(panel_current, color_darkblue, 0);
        lv_obj_set_style_border_width(panel_current, 0, 0);
        lv_obj_set_style_radius(panel_current, 16, 0);

        label_current = lv_label_create(panel_current);
        lv_obj_set_style_text_color(label_current, color_white, 0);
        lv_obj_center(label_current);
        lv_label_set_text(label_current, "0.0 A");

        // --- BOTTOM RIGHT (ESC Temp) ---
        lv_obj_t* panel_temp = lv_obj_create(lv_scr_act());
        lv_obj_set_size(panel_temp, 140, 60);
        lv_obj_align(panel_temp, LV_ALIGN_BOTTOM_RIGHT, -20, -20);
        lv_obj_set_style_bg_color(panel_temp, color_darkblue, 0);
        lv_obj_set_style_border_width(panel_temp, 0, 0);
        lv_obj_set_style_radius(panel_temp, 16, 0);

        label_temp = lv_label_create(panel_temp);
        lv_obj_set_style_text_color(label_temp, color_white, 0);
        lv_obj_center(label_temp);
        lv_label_set_text(label_temp, "25.0 C");
    }

    void update() {
        int32_t erpm = g_vehicle_state.erpm;
        float v = g_vehicle_state.battery_voltage_v;
        float a = g_vehicle_state.battery_current_a;
        float temp = g_vehicle_state.mosfet_temp_c;

        lv_arc_set_value(arc_erpm, erpm);
        lv_label_set_text_fmt(label_erpm, "%d\nERPM", (int)erpm);

        float pct = ((v - 32.0f) / (42.0f - 32.0f)) * 100.0f;
        if(pct < 0) pct = 0;
        if(pct > 100) pct = 100;

        lv_bar_set_value(bar_battery, (int)pct, LV_ANIM_ON);
        lv_label_set_text_fmt(label_battery, "%.1fV", v);

        // Warning Dynamics (Red overrides)
        if(v < 32.0f) {
            lv_obj_set_style_text_color(label_battery, lv_color_hex(0xFF3333), 0);
            lv_obj_set_style_bg_color(bar_battery, lv_color_hex(0xFF3333), LV_PART_INDICATOR);
        } else {
            lv_obj_set_style_text_color(label_battery, lv_color_hex(0xFFFFFF), 0);
            lv_obj_set_style_bg_color(bar_battery, lv_color_hex(0xA020F0), LV_PART_INDICATOR);
        }

        lv_label_set_text_fmt(label_current, "%.1f A", a);

        lv_label_set_text_fmt(label_temp, "%.1f C", temp);
        if(temp > 75.0f) {
            lv_obj_set_style_text_color(label_temp, lv_color_hex(0xFF3333), 0);
        } else {
            lv_obj_set_style_text_color(label_temp, lv_color_hex(0xFFFFFF), 0);
        }
    }
}
