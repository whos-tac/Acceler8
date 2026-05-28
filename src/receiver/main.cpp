#include <Arduino.h>
#include "receiver_app.h"

#ifdef ARDUINO
void setup() {
    ReceiverApp::init();
}

void loop() {
    ReceiverApp::update();
}
#endif
