#include "underglow_controller.h"
#include "can_driver.h"
#include "settings_screen.h"

#ifdef ARDUINO
#include <Adafruit_NeoPixel.h>

const uint16_t PixelCount = 180; // Default for 3m strips (60 LED/m)
const uint8_t PixelPin = 16; // Using Terminal Pin 8 (GIO) instead of fried IO4

Adafruit_NeoPixel strip(PixelCount, PixelPin, NEO_GRB + NEO_KHZ800);

// ACCELER8 Color Palette
const uint8_t colorPurpleR = 195, colorPurpleG = 177, colorPurpleB = 225; // 0xC3B1E1
const uint8_t colorCyanR = 0, colorCyanG = 204, colorCyanB = 204;     // 0x00CCCC

#endif

namespace UnderglowController {

void init() {
  // Initialize state defaults
  g_vehicle_state.led_r = 195;
  g_vehicle_state.led_g = 177;
  g_vehicle_state.led_b = 225;
  g_vehicle_state.led_brightness = 255;
  g_vehicle_state.led_mode = 2; // Default to "Breathing"

#ifdef ARDUINO
  strip.begin();

  // Diagnostic boot flash (Solid White for 500ms)
  for (uint16_t i = 0; i < PixelCount; i++) {
    strip.setPixelColor(i, strip.Color(255, 255, 255));
  }
  strip.show();
  delay(500);
#endif
}

void update() {
  static uint32_t last_update = 0;
#ifdef ARDUINO
  uint32_t now = millis();

  // Non-blocking update (approx 60fps)
  if (now - last_update < 16)
    return;
  last_update = now;

  DASH_LOCK();
  uint8_t mode = g_vehicle_state.led_mode;
  uint8_t r = g_vehicle_state.led_r;
  uint8_t g = g_vehicle_state.led_g;
  uint8_t b = g_vehicle_state.led_b;
  uint8_t brightness = g_vehicle_state.led_brightness;
  float speed_kmh = g_vehicle_state.speed_kmh;
  DASH_UNLOCK();

  if (SettingsScreen::is_active()) {
    uint16_t hue = (uint16_t)((SettingsScreen::underglow_hue / 360.0f) * 65535.0f);
    uint8_t val = (uint8_t)((SettingsScreen::display_brightness / 100.0f) * 255.0f);
    uint32_t rgb = strip.ColorHSV(hue, 255, val);
    
    for (uint16_t i = 0; i < PixelCount; i++) {
      strip.setPixelColor(i, rgb);
    }
    strip.show();
    return;
  }

  if (mode == 0) {
    // OFF
    set_all(0, 0, 0);
  } else if (mode == 1) {
    // SOLID
    float brightness_f = brightness / 255.0f;
    set_all((uint8_t)(r * brightness_f), (uint8_t)(g * brightness_f),
            (uint8_t)(b * brightness_f));
  } else if (mode == 2) {
    // BREATHING EFFECT (Purple -> Cyan)
    static float angle = 0;
    angle += 0.02f;
    if (angle > 2 * PI)
      angle = 0;

    float intensity = (sin(angle) + 1.0f) / 2.0f; // 0.0 to 1.0

    uint8_t r_blend = colorPurpleR + (colorCyanR - colorPurpleR) * intensity;
    uint8_t g_blend = colorPurpleG + (colorCyanG - colorPurpleG) * intensity;
    uint8_t b_blend = colorPurpleB + (colorCyanB - colorPurpleB) * intensity;

    // Apply brightness global scale
    float brightness_f = brightness / 255.0f;
    r_blend = (uint8_t)(r_blend * brightness_f);
    g_blend = (uint8_t)(g_blend * brightness_f);
    b_blend = (uint8_t)(b_blend * brightness_f);

    uint32_t blendedColor = strip.Color(r_blend, g_blend, b_blend);

    for (uint16_t i = 0; i < PixelCount; i++) {
      strip.setPixelColor(i, blendedColor);
    }
    strip.show();
  } else if (mode == 3) {
    // SPEED REACTIVE
    // Color shifts from Cyan (Slow) to Purple (Fast)
    float speed_factor = speed_kmh / 40.0f; // Scale to 40km/h
    if (speed_factor > 1.0f)
      speed_factor = 1.0f;

    uint8_t r_blend = colorCyanR + (colorPurpleR - colorCyanR) * speed_factor;
    uint8_t g_blend = colorCyanG + (colorPurpleG - colorCyanG) * speed_factor;
    uint8_t b_blend = colorCyanB + (colorPurpleB - colorCyanB) * speed_factor;

    float brightness_f = brightness / 255.0f;
    r_blend = (uint8_t)(r_blend * brightness_f);
    g_blend = (uint8_t)(g_blend * brightness_f);
    b_blend = (uint8_t)(b_blend * brightness_f);

    uint32_t speed_color = strip.Color(r_blend, g_blend, b_blend);

    for (uint16_t i = 0; i < PixelCount; i++) {
      strip.setPixelColor(i, speed_color);
    }
    strip.show();
  }
#endif
}

void set_all(uint8_t r, uint8_t g, uint8_t b) {
#ifdef ARDUINO
  uint32_t color = strip.Color(r, g, b);
  for (uint16_t i = 0; i < PixelCount; i++) {
    strip.setPixelColor(i, color);
  }
  strip.show();
#endif
}
} // namespace UnderglowController
