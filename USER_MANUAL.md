# ACCELER8 DashBoard & Remote: Rider's Manual

Welcome to the **ACCELER8 Tech Stack**. This manual covers everything you need to know about your dashboard, remote control, and on-board features to ensure a safe, customized, and high-performance ride.

---

## 1. The Dashboard UI

The ACCELER8 Dashboard is designed around **Brutalist Minimalism** for maximum "glanceability" while riding. Critical metrics use large, pixel-art typography and high-contrast colors (Purple/Cyan on Black) so you can read them effortlessly at high speeds and under heavy vibration.

**Main Display Elements:**
*   **Speed**: Displayed in massive 300px typography on the top left. The "KM/H" tag is omitted to make the number as large as possible.
*   **Power (Watts)**: Located just below the speed, showing your real-time power consumption and regeneration.
*   **Secondary Stats**: The right side displays stacked metrics:
    *   **TRIP**: Distance traveled on the current ride.
    *   **RANGE**: Estimated remaining battery range.
    *   **Wh/km**: Your real-time efficiency.
    *   **ESC Temp**: Electronic Speed Controller temperature, shown as a horizontal bar to easily spot thermal changes.
*   **Battery Strip**: A 10-segment color gradient bar across the bottom (Red → Orange → Yellow → Green → Cyan) for a quick visual on remaining juice.

---

## 2. Remote Control Basics

The remote is your primary interface while moving. It features a D-Pad and a CONFIRM button.

*   **Horn**: During normal riding, pressing the **CONFIRM** button sounds the electronic horn.
*   **Accessing Settings**: Hold **UP + DOWN** on the D-Pad simultaneously for 2 seconds to enter the Settings Menu. 
*   **Menu Navigation**: Once the Settings Menu is open, use the D-Pad (Up, Down, Left, Right) to navigate and change values. Use the **CONFIRM** button as "Enter" to select or save items.

---

## 3. Settings Menu (D-Pad)

When you hold **UP + DOWN** to enter the Settings Menu, your ESC will automatically **ignore all throttle inputs**. This is a critical safety feature to prevent accidental acceleration while tuning your board.

In the D-Pad Settings Menu, you can configure:
*   **Mechanical Configurations**: Adjust `Pole Pairs`, `Gear Ratio`, and `Wheel Diameter`. Setting these correctly ensures your speed and distance metrics are perfectly accurate.
*   **Odometer Reset**: Select the Odometer and press the **CONFIRM** button to reset your total distance.
*   **Touch Settings Access**: Select "Touch Settings" to unlock the dashboard's capacitive touch screen.

---

## 4. Touch Settings Modal

To prevent accidental inputs from bumps, water drops, or vibrations while riding, the dashboard's touch screen is **completely disabled by default**. You must open the "Touch Settings" modal via the D-Pad to interact with the screen.

Inside the Touch Settings modal, you can tap and adjust:
*   **ESC Gear**: Change your acceleration profile (None, Low, Medium, High).
*   **ESC Direction**: Toggle motor direction (Forward, Reverse).
*   **Headlight**: Turn the front headlights on or off.
*   **Brightness**: Slide to adjust the dashboard backlight brightness (10% - 100%).
*   **Underglow Color**: Use the interactive color wheel to pick your custom underglow hue.

*Remember: Selecting "Save & Exit" commits your mechanical changes to the board's memory so they persist after restarting.*

---

## 5. Underglow Lighting

Your board is equipped with a dynamic LED underglow system. The lighting responds to your speed and settings.

**Lighting Modes:**
*   **Off**: LEDs completely disabled.
*   **Solid**: Constant color based on your selection in the Touch Settings.
*   **Breathing**: The default mode. Smoothly pulses back and forth between Purple and Cyan.
*   **Speed Reactive**: Changes color dynamically as you ride. Glows **Solid Cyan** at a standstill (0 km/h), and smoothly transitions to **Solid Purple** as you hit high speeds (40+ km/h).

---

## 6. Safety Features & Alerts

The ACCELER8 system constantly monitors critical hardware via the CAN bus and wireless links. If an issue is detected, it will instantly display a full-screen, high-visibility red alert.

*   **Settings Throttle Cut**: As mentioned, entering the Settings Menu broadcasts a signal to the receiver to cut all throttle inputs. You cannot accelerate while the menu is open.
*   **Remote Disconnect**: If the wireless connection between your remote and the receiver drops, a full-screen warning covers the dash. Throttle is inherently cut.
*   **CAN Timeout**: Warns you if the dashboard loses communication with the underlying motor controllers.
*   **Overtemp**: Alerts you if the ESC or motors exceed safe operating temperatures, telling you to pull over and let the system cool down before causing hardware damage.

Ride safe, wear a helmet, and enjoy the brutalist experience!
