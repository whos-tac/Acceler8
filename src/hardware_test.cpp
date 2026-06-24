#include <Arduino.h>
#include <Wire.h>
#include <Arduino_GFX_Library.h>

#define SCREEN_WIDTH 480
#define SCREEN_HEIGHT 480

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

void setup() {
    Serial.begin(115200);
    delay(100);

    // Explicitly stabilize SPI pins BEFORE resetting the ST7701
    pinMode(42, OUTPUT);
    digitalWrite(42, HIGH); // CS MUST be HIGH during reset
    pinMode(2, OUTPUT);
    digitalWrite(2, LOW);   // SCK
    pinMode(1, OUTPUT);
    digitalWrite(1, LOW);   // MOSI

    delay(1500);

    Wire.begin(15, 7);

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
        Wire.beginTransmission(0x24); Wire.write(0x02); Wire.write(0xff); Wire.endTransmission();
        Wire.beginTransmission(0x24); Wire.write(0x01); Wire.write(0x00); Wire.endTransmission();
        Wire.beginTransmission(0x24); Wire.write(0x03); Wire.write(0x30); Wire.endTransmission();
        delay(20);
        Wire.beginTransmission(0x24); Wire.write(0x01); Wire.write(0x0a); Wire.endTransmission();
        delay(120);
        Wire.beginTransmission(0x24); Wire.write(0x03); Wire.write(0x3a); Wire.endTransmission();
    }

    gfx->begin();
    gfx->fillScreen(RED);
    delay(1000);
    gfx->fillScreen(GREEN);
    delay(1000);
    gfx->fillScreen(BLUE);
    delay(1000);
    gfx->fillScreen(BLACK);
    
    gfx->setTextSize(4);
    gfx->setTextColor(WHITE);
    gfx->setCursor(100, 200);
    gfx->println("I AM ALIVE!");
}

void loop() {
    delay(1000);
}
