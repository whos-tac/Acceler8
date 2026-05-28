#pragma once

// Global odometer value (in km)
extern float total_distance;

class DashApp {
public:
    static void init();
    static void update();
};
