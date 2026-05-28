#include "mechanical_config.h"

// Fallback mechanical configuration variables (can be modified by Settings UI)
int motor_pole_pairs = 7;
float gear_ratio = 4.0f;
float wheel_diameter_mm = 200.0f;

#ifdef ARDUINO
#include <Preferences.h>
static Preferences mech_prefs;
#endif

void load_mechanical_config() {
#ifdef ARDUINO
    mech_prefs.begin("mech_cfg", false);
    motor_pole_pairs = mech_prefs.getInt("pole_pairs", 7);
    gear_ratio = mech_prefs.getFloat("gear_ratio", 4.0f);
    wheel_diameter_mm = mech_prefs.getFloat("wheel_dia", 200.0f);
#endif
}

void save_mechanical_config() {
#ifdef ARDUINO
    mech_prefs.putInt("pole_pairs", motor_pole_pairs);
    mech_prefs.putFloat("gear_ratio", gear_ratio);
    mech_prefs.putFloat("wheel_dia", wheel_diameter_mm);
#endif
}
