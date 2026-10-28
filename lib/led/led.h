#ifndef LED_H
#define LED_H

#include <Arduino.h>

namespace Led {
enum class State : uint8_t {
    NORMAL,     // Green: ready / normal operation
    ACTIVITY,   // Blue: measuring, calculating, or a result is available
    WARNING,    // Yellow: recoverable fault / peripheral unavailable
    CRITICAL    // Red: critical fault (e.g. MAX30102 initialization failure)
};

void begin();
void setState(State state);
State state();
void update();
}

#endif
