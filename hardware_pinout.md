# Hardware Pinout

Template for Waveshare ESP32-S3-Touch-LCD-4 (Rev 4.0) pinouts.

## Display Driver Setup
- **Driver**: ST7701
- Requires an RGB interface setup with PSRAM enabled.

## Pin Definitions
### RGB LCD Display (ST7701)
```c
#define LCD_DE     40
#define LCD_VSYNC  39
#define LCD_HSYNC  38
#define LCD_PCLK   41

#define LCD_R0 46
#define LCD_R1 3
#define LCD_R2 8
#define LCD_R3 18
#define LCD_R4 17

#define LCD_G0 14
#define LCD_G1 13
#define LCD_G2 12
#define LCD_G3 11
#define LCD_G4 10
#define LCD_G5 9

#define LCD_B0 5
#define LCD_B1 45
#define LCD_B2 48
#define LCD_B3 47
#define LCD_B4 21
```

### CAN Bus (TWAI)
```c
// Based on ESP-IDF transceiver configuration
#define CAN_TX_PIN 6
#define CAN_RX_PIN 0
```

### Touch Controller (GT911) & IO Expander
```c
// I2C interface routed to Touch IC and TCA9554PWR (Address 0x24)
#define TOUCH_SCL_PIN 7
#define TOUCH_SDA_PIN 15
```
