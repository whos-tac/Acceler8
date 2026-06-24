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
        // Wait for power supply and capacitors to stabilize on cold start.
        // Tripled to 1500ms to guarantee stabilization on cold boot.
        delay(1500);

        Wire.begin(15, 7);

        // ponytail: Probe address 0x24 up to 150 times (1500ms) to wait for IO Expander to boot
        int retry = 150;
        bool expander_ready = false;
        while (retry > 0) {
            Wire.beginTransmission(0x24);
            if (Wire.endTransmission() == 0) {
                expander_ready = true;
                break;
            }
            delay(10);
            retry--;
        }

        if (expander_ready) {
            // Set polarity inversion register (Register 0x02)
            Wire.beginTransmission(0x24); Wire.write(0x02); Wire.write(0xff); Wire.endTransmission();

            // Drive all outputs LOW:
            // Pin 0 (VBAT_5V Enable) = LOW (enables Q4 MOSFET to power 5V subsystems/backlight)
            // Pin 1 (TP_RST) = LOW (assert touch reset)
            // Pin 2 (TP_INT) = LOW (holds touch INT low for GT911 address selection)
            // Pin 3 (LCD_RST) = LOW (assert LCD reset)
            Wire.beginTransmission(0x24); Wire.write(0x01); Wire.write(0x00); Wire.endTransmission();

            // ponytail: Set Configuration register to 0x30 to configure Pins 0-3 as outputs for power and reset control
            Wire.beginTransmission(0x24); Wire.write(0x03); Wire.write(0x30); Wire.endTransmission();

            // Hold resets LOW for 250ms to stabilize power rails and reset display/touch controllers
            delay(250);

            // Release resets:
            // Pin 0 (VBAT_5V Enable) = LOW (remain ON)
            // Pin 1 (TP_RST) = HIGH (release touch reset)
            // Pin 2 (TP_INT) = LOW (GT911 requires INT to be held low during reset release)
            // Pin 3 (LCD_RST) = HIGH (release LCD reset)
            // Output register value: binary 0000 1010 = 0x0a
            Wire.beginTransmission(0x24); Wire.write(0x01); Wire.write(0x0a); Wire.endTransmission();

            // Wait 250ms to allow ST7701 and GT911 internal calibration to finish
            delay(250);

            // ponytail: Restore original vendor pin direction configuration (0x3a) to return resets to inputs
            Wire.beginTransmission(0x24); Wire.write(0x03); Wire.write(0x3a); Wire.endTransmission();
        } else {
            Serial.println("Warning: TCA9554 IO Expander at 0x24 not responding!");
        }

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
