#include <Arduino.h>
#include <TFT_eSPI.h>

TFT_eSPI tft = TFT_eSPI();

// LilyGo T-Display S3 Pin Definitions
#define PIN_POT 1
#define PIN_BTN_UP 2
#define PIN_BTN_DOWN 3
#define PIN_BTN_LEFT 10
#define PIN_BTN_RIGHT 11
#define PIN_BTN_CONFIRM 12
#define PIN_POWER_ON 15
#define PIN_TFT_BL 38
#define PIN_BAT_ADC 4

struct Button {
    int pin;
    const char* name;
    bool lastState;
};

Button buttons[] = {
    {PIN_BTN_UP, "UP", true},
    {PIN_BTN_DOWN, "DOWN", true},
    {PIN_BTN_LEFT, "LEFT", true},
    {PIN_BTN_RIGHT, "RIGHT", true},
    {PIN_BTN_CONFIRM, "CONFIRM", true}
};

const int numButtons = sizeof(buttons) / sizeof(Button);

void setup() {
    Serial.begin(115200);
    
    // CRITICAL: Pull GPIO 15 HIGH to keep board alive on battery power!
    pinMode(PIN_POWER_ON, OUTPUT);
    digitalWrite(PIN_POWER_ON, HIGH);
    
    // Init Backlight
    pinMode(PIN_TFT_BL, OUTPUT);
    digitalWrite(PIN_TFT_BL, HIGH);

    // Setup Pins
    pinMode(PIN_POT, INPUT);
    for (int i = 0; i < numButtons; i++) {
        pinMode(buttons[i].pin, INPUT_PULLUP);
    }

    // Setup UI - Oriented vertically (170 width, 320 height)
    tft.init();
    tft.setRotation(0); 
    tft.fillScreen(TFT_BLACK);
}

void loop() {
    static uint32_t lastUpdate = 0;
    
    // Update display every 50ms (20Hz)
    if (millis() - lastUpdate > 50) {
        lastUpdate = millis();

        // 1. Read Potentiometer & Battery
        int potVal = analogRead(PIN_POT);
        int rawBat = analogRead(PIN_BAT_ADC);
        float voltage = (rawBat / 4095.0f) * 3.3f * 2.0f;

        // Clear screen to avoid overlapping
        tft.fillScreen(TFT_BLACK);
        
        // Header
        tft.setTextColor(TFT_CYAN, TFT_BLACK);
        tft.setTextSize(2);
        tft.setCursor(10, 10);
        tft.println("REMOTE PIN HUD");
        tft.drawFastHLine(0, 30, 170, TFT_WHITE);

        // 2. Display Potentiometer Reading
        tft.setTextSize(1);
        tft.setCursor(10, 42);
        tft.setTextColor(TFT_YELLOW, TFT_BLACK);
        tft.printf("Potentiometer: %4d", potVal);

        // Simple ASCII Progress Bar for throttle visualization
        int barLength = (potVal * 16) / 4095; // 0 to 16 blocks
        tft.setCursor(10, 56);
        tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
        tft.print("[");
        tft.setTextColor(TFT_YELLOW, TFT_BLACK);
        for (int b = 0; b < 16; b++) {
            if (b < barLength) tft.print("#");
            else tft.print("-");
        }
        tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
        tft.print("]");

        // Divider
        tft.drawFastHLine(0, 75, 170, TFT_DARKGREY);

        // 3. Display Button States with plenty of vertical room (Y = 85 to 215)
        for (int i = 0; i < numButtons; i++) {
            bool currentState = digitalRead(buttons[i].pin);
            
            tft.setCursor(10, 88 + (i * 26)); // Spacing of 26px provides clean spacing
            tft.setTextSize(1);
            if (currentState == LOW) { // Pressed
                tft.setTextColor(TFT_GREEN, TFT_BLACK);
                tft.printf("%-7s: [ PRESSED ]", buttons[i].name);
                
                if (buttons[i].lastState == HIGH) {
                    Serial.printf("[BUTTON] %s Pressed!\n", buttons[i].name);
                }
            } else {
                tft.setTextColor(TFT_WHITE, TFT_BLACK);
                tft.printf("%-7s: [ idle    ]", buttons[i].name);
            }
            buttons[i].lastState = currentState;
        }

        // Divider
        tft.drawFastHLine(0, 226, 170, TFT_DARKGREY);

        // 4. Display Battery Status (Bottom HUD)
        tft.setCursor(10, 238);
        tft.setTextSize(1);
        tft.setTextColor(TFT_MAGENTA, TFT_BLACK);
        tft.printf("Battery: %.2fV", voltage);
        
        tft.setCursor(10, 252);
        tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
        tft.printf("Raw ADC: %d", rawBat);
        
        tft.setCursor(10, 270);
        if (voltage < 3.3f) {
            tft.setTextColor(TFT_RED, TFT_BLACK);
            tft.println("BATT LEVEL: CRITICAL!");
        } else {
            tft.setTextColor(TFT_GREEN, TFT_BLACK);
            tft.println("BATT LEVEL: OK");
        }
    }
}
