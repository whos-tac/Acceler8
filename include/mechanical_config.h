#pragma once
#include <stdint.h>
#include <math.h>

// ESK8 Mechanical Configuration
// Defaults set for standard MTB (Mountain Board) setup

#define MOTOR_POLE_PAIRS 7
#define GEAR_RATIO 4.0f           // e.g., 15T Motor to 60T Wheel
#define WHEEL_DIAMETER_MM 200.0f  // 8-inch pneumatic tires

#ifndef PI
#define PI 3.14159265358979323846
#endif

// Helper macro to calculate Speed (km/h) from ERPM
inline float calculate_speed_kmh(int32_t erpm) {
    float rpm = (float)erpm / MOTOR_POLE_PAIRS;
    float wheel_rpm = rpm / GEAR_RATIO;
    float circumference_m = (WHEEL_DIAMETER_MM * PI) / 1000.0f;
    return (wheel_rpm * circumference_m * 60.0f) / 1000.0f;
}
