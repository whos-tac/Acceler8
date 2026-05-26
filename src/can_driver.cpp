#include "can_driver.h"
#include <Arduino.h>
#include <math.h>

VehicleState g_vehicle_state = {0};

namespace CANDriver {

    void init() {
        // Simulator Initialization: Base values
        g_vehicle_state.battery_voltage_v = 42.0f;
        g_vehicle_state.mosfet_temp_c = 25.0f;
        g_vehicle_state.erpm = 0;
        g_vehicle_state.battery_current_a = 0.0f;
    }

    void poll() {
        // Continuous smooth simulation layer over time
        float t = millis() / 2000.0f; 
        
        // Sine wave scaling for ERPM
        float load = sin(t); 
        g_vehicle_state.erpm = (int32_t)(fabs(load) * 45000.0f);
        
        // Regenerative vs Active Load Current Simulation
        if (load > 0) {
            g_vehicle_state.battery_current_a = load * 25.0f; 
        } else {
            g_vehicle_state.battery_current_a = load * 8.0f; 
        }

        // Apply load-based voltage sag and thermal increases
        g_vehicle_state.battery_voltage_v = 41.5f - (g_vehicle_state.battery_current_a * 0.1f) - (millis() / 150000.0f);
        g_vehicle_state.mosfet_temp_c = 30.0f + (fabs(load) * 55.0f);
    }

}
