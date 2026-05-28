#pragma once

// Global odometer value (in km)
extern float total_distance;

class Odometer {
public:
    static void init();
    static void update();
    static void save_if_needed(); // force save if there are unsaved changes
    static void reset();
};
