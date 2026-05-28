#pragma once
#include <stdint.h>
#include <math.h>

// ESK8 Mechanical Configuration
extern int motor_pole_pairs;
extern float gear_ratio;
extern float wheel_diameter_mm;

void load_mechanical_config();
void save_mechanical_config();

#ifdef ARDUINO
#include <Arduino.h>
#endif

#ifndef PI
#define PI 3.14159265358979323846
#endif

// Helper macro to calculate Speed (km/h) from ERPM
inline float calculate_speed_kmh(int32_t erpm) {
    if (motor_pole_pairs == 0 || gear_ratio == 0.0f) return 0.0f;
    float rpm = (float)erpm / motor_pole_pairs;
    float wheel_rpm = rpm / gear_ratio;
    float circumference_m = (wheel_diameter_mm * PI) / 1000.0f;
    return (wheel_rpm * circumference_m * 60.0f) / 1000.0f;
}
