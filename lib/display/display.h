#ifndef DISPLAY_H
#define DISPLAY_H
#include <Arduino.h>
#include "../mode/mode.h"
#include "../storage/storage.h"
namespace Display {
bool begin();
bool available();
void update(Mode::Screen screen, Mode::MeasureMode mode, const char *message,
            const char *timestamp, bool rtcAvailable, float hb, bool hbAvailable,
            const char *measurementStatus, uint8_t samplesPercent, uint8_t menuSelection);
}
#endif
