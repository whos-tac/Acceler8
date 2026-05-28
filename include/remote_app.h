#pragma once

#include <stdint.h>

class RemoteApp {
public:
    static void init();
    static void update();

#ifndef ARDUINO
    static void set_sim_pot_val(int val);
#endif
};
