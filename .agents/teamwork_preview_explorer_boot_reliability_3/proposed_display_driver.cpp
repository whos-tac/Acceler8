#include "display_driver.h"
#include <lvgl.h>

#define SCREEN_WIDTH 480
#define SCREEN_HEIGHT 480

#ifdef ARDUINO
#include <Arduino_GFX_Library.h>
#include <Wire.h>
#include <TAMC_GT911.h>
#include "ui_controller.h"

// Touch IC Configuration
#define TOUCH_SDA  15
#define TOUCH_SCL  7
#define TOUCH_INT  -1
#define TOUCH_RST  -1
#define TOUCH_WIDTH 480
#define TOUCH_HEIGHT 480

TAMC_GT911 tp = TAMC_GT911(TOUCH_SDA, TOUCH_SCL, TOUCH_INT, TOUCH_RST, TOUCH_WIDTH, TOUCH_HEIGHT);

// Using ST7701 Waveshare custom profile
Arduino_DataBus *bus = new Arduino_SWSPI(GFX_NOT_DEFINED /* DC */, 42 /* CS */, 2 /* SCK */, 1 /* MOSI */, GFX_NOT_DEFINED /* MISO */);

Arduino_ESP32RGBPanel *rgbpanel = new Arduino_ESP32RGBPanel(
  40 /* DE */, 39 /* VSYNC */, 38 /* HSYNC */, 41 /* PCLK */,
  46 /* R0 */, 3 /* R1 */, 8 /* R2 */, 18 /* R3 */, 17 /* R4 */,
  14 /* G0 */, 13 /* G1 */, 12 /* G2 */, 11 /* G3 */, 10 /* G4 */, 9 /* G5 */,
  5 /* B0 */, 45 /* B1 */, 48 /* B2 */, 47 /* B3 */, 21 /* B4 */,
  1 /* hsync_poly */, 10, 8, 50,
  1 /* vsync_poly */, 10, 8, 20);

Arduino_RGB_Display *gfx = new Arduino_RGB_Display(
  SCREEN_WIDTH, SCREEN_HEIGHT, rgbpanel, 0 /* rotation */, true /* auto_flush */,
  bus, GFX_NOT_DEFINED /* RST */, st7701_type1_init_operations, sizeof(st7701_type1_init_operations));

static lv_disp_draw_buf_t draw_buf;
static lv_color_t *buf1;

void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);
#if (LV_COLOR_16_SWAP != 0)
    gfx->draw16bitBeRGBBitmap(area->x1, area->y1, (uint16_t *)&color_p->full, w, h);
#else
    gfx->draw16bitRGBBitmap(area->x1, area->y1, (uint16_t *)&color_p->full, w, h);
#endif
    lv_disp_flush_ready(disp);
}

static void my_touchpad_read(lv_indev_drv_t * indev_driver, lv_indev_data_t * data) {
    if (!UIController::touch_enabled) {
        data->state = LV_INDEV_STATE_REL;
        return;
    }

    tp.read();
    if (tp.isTouched) {
        data->state = LV_INDEV_STATE_PR;
        data->point.x = tp.points[0].x;
        data->point.y = tp.points[0].y;
    } else {
        data->state = LV_INDEV_STATE_REL;
    }
}

namespace DisplayDriver {
    void init() {
        // Wait for power supply to stabilize on cold start
        delay(100);

        Wire.begin(15, 7);
        delay(10); // Let I2C bus settle

        // Set polarity inversion register (Register 0x02)
        Wire.beginTransmission(0x24); Wire.write(0x02); Wire.write(0xff); Wire.endTransmission();

        // Configure TCA9554 IO Expander Direction Register (Register 0x03)
        // P0 (LCD_RST): Output (0)
        // P1 (TP_INT): Input (1)
        // P2 (LCD_BL): Output (0)
        // P3 (TP_RST): Output (0)
        // P4-P5: Input (1)
        // P6-P7: Output (0)
        // ponytail: configure TCA9554 to set P3 (TP_RST) as output (0x32 direction byte instead of original 0x3a) for proper hardware reset of touch controller
        Wire.beginTransmission(0x24); Wire.write(0x03); Wire.write(0x32); Wire.endTransmission();

        // Pull LCD_RST (P0) LOW, TP_RST (P3) LOW, and keep LCD_BL (P2) LOW
        // Output Port Register (0x01) value: 1100 0000 = 0xC0
        Wire.beginTransmission(0x24); Wire.write(0x01); Wire.write(0xC0); Wire.endTransmission();
        delay(20); // Keep reset active for 20ms

        // Release Reset: Drive LCD_RST (P0) and TP_RST (P3) HIGH, keep LCD_BL (P2) LOW
        // Output Port Register (0x01) value: 1100 1001 = 0xC9
        Wire.beginTransmission(0x24); Wire.write(0x01); Wire.write(0xC9); Wire.endTransmission();
        
        // Wait 120ms for display controller internal stabilization before commands
        delay(120);

        gfx->begin();
        lv_init();

        buf1 = (lv_color_t *)heap_caps_malloc(SCREEN_WIDTH * SCREEN_HEIGHT / 10 * sizeof(lv_color_t), MALLOC_CAP_DMA);
        lv_disp_draw_buf_init(&draw_buf, buf1, NULL, SCREEN_WIDTH * SCREEN_HEIGHT / 10);

        static lv_disp_drv_t disp_drv;
        lv_disp_drv_init(&disp_drv);
        disp_drv.hor_res = SCREEN_WIDTH;
        disp_drv.ver_res = SCREEN_HEIGHT;
        disp_drv.flush_cb = my_disp_flush;
        disp_drv.draw_buf = &draw_buf;
        lv_disp_drv_register(&disp_drv);

        tp.begin();
        tp.setRotation(ROTATION_NORMAL);

        // Turn on the LCD Backlight (LCD_BL = P2 HIGH)
        // Output Port Register (0x01) value: 1111 1101 = 0xFD or 0xFF
        Wire.beginTransmission(0x24); Wire.write(0x01); Wire.write(0xFF); Wire.endTransmission();

        static lv_indev_drv_t indev_drv;
        lv_indev_drv_init(&indev_drv);
        indev_drv.type = LV_INDEV_TYPE_POINTER;
        indev_drv.read_cb = my_touchpad_read;
        lv_indev_drv_register(&indev_drv);
    }

    void tick() {
        lv_timer_handler();
    }
}

#else

// ---------------- Native Desktop Driver with SDL ---------------- //
#define SDL_MAIN_HANDLED /* To fix Windows entry points */
#include <SDL2/SDL.h>
#include "sdl/sdl.h"

namespace DisplayDriver {
    void init() {
#ifndef NATIVE_FULL_STACK
        lv_init();
        
        sdl_init();

        static lv_disp_draw_buf_t draw_buf;
        static lv_color_t buf1[SCREEN_WIDTH * SCREEN_HEIGHT / 10];
        lv_disp_draw_buf_init(&draw_buf, buf1, NULL, SCREEN_WIDTH * SCREEN_HEIGHT / 10);

        static lv_disp_drv_t disp_drv;
        lv_disp_drv_init(&disp_drv);
        disp_drv.hor_res = SCREEN_WIDTH;
        disp_drv.ver_res = SCREEN_HEIGHT;
        disp_drv.flush_cb = sdl_display_flush;
        disp_drv.draw_buf = &draw_buf;
        lv_disp_drv_register(&disp_drv);

        static lv_indev_drv_t indev_drv;
        lv_indev_drv_init(&indev_drv);
        indev_drv.type = LV_INDEV_TYPE_POINTER;
        indev_drv.read_cb = sdl_mouse_read;
        lv_indev_drv_register(&indev_drv);
#endif
    }

    void tick() {
#ifndef NATIVE_FULL_STACK
        lv_timer_handler();
#endif
    }
}

#endif
