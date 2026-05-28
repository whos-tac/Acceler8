#include "underglow_controller.h"
#include "can_driver.h"

#ifdef ARDUINO
#include <NeoPixelBus.h>

// Configuration
const uint16_t PixelCount = 120; // Default for 2x 1m strips (60 LED/m)
const uint8_t PixelPin = 4;      // MISO pin on SD card port

// WS2815 requires a >250us reset pulse (unlike WS2812), so we must use the
// WS2813 method.
NeoPixelBus<NeoGrbFeature, NeoWs2813Method> strip(PixelCount, PixelPin);

// ACCELER8 Color Palette
RgbColor colorPurple(195, 177, 225); // 0xC3B1E1
RgbColor colorCyan(0, 204, 204);     // 0x00CCCC

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
  strip.Begin();
  strip.Show();

  pinMode(PixelPin, OUTPUT); // Diagnostic: Force pin mode
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

  if (g_vehicle_state.led_mode == 0) {
    // OFF
    set_all(0, 0, 0);
  } else if (g_vehicle_state.led_mode == 1) {
    // SOLID
    set_all(g_vehicle_state.led_r, g_vehicle_state.led_g,
            g_vehicle_state.led_b);
  } else if (g_vehicle_state.led_mode == 2) {
    // BREATHING EFFECT (Purple -> Cyan)
    static float angle = 0;
    angle += 0.02f;
    if (angle > 2 * PI)
      angle = 0;

    float intensity = (sin(angle) + 1.0f) / 2.0f; // 0.0 to 1.0

    RgbColor blended = RgbColor::LinearBlend(colorPurple, colorCyan, intensity);

    // Apply brightness global scale
    float brightness_f = g_vehicle_state.led_brightness / 255.0f;
    blended = blended.Dim((uint8_t)(brightness_f * 255));

    for (uint16_t i = 0; i < PixelCount; i++) {
      strip.SetPixelColor(i, blended);
    }
    strip.Show();
  } else if (g_vehicle_state.led_mode == 3) {
    // SPEED REACTIVE
    // Color shifts from Cyan (Slow) to Purple (Fast)
    float speed_factor = g_vehicle_state.speed_kmh / 40.0f; // Scale to 40km/h
    if (speed_factor > 1.0f)
      speed_factor = 1.0f;

    RgbColor speed_color =
        RgbColor::LinearBlend(colorCyan, colorPurple, speed_factor);

    for (uint16_t i = 0; i < PixelCount; i++) {
      strip.SetPixelColor(i, speed_color);
    }
    strip.Show();
  }
#endif
}

void set_all(uint8_t r, uint8_t g, uint8_t b) {
#ifdef ARDUINO
  RgbColor color(r, g, b);
  for (uint16_t i = 0; i < PixelCount; i++) {
    strip.SetPixelColor(i, color);
  }
  strip.Show();
#endif
}
} // namespace UnderglowController
