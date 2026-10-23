#ifndef MODE_H
#define MODE_H
#include <Arduino.h>
#include "../buttons/buttons.h"
#include "../storage/storage.h"
namespace Mode {
enum class Screen : uint8_t { HOME, MEASURE, LOGS, SETTINGS, CLEAR_CONFIRM, MESSAGE };
enum class MeasureMode : uint8_t { CONTINUOUS, SINGLE };
void begin();
void handle(Buttons::Event event);
Screen screen();
MeasureMode measureMode();
void setMessage(const char *message, uint32_t durationMs = 2200);
const char *message();
bool measurementRequested();
void clearMeasurementRequest();
bool measurementActive();
bool singleMeasurementComplete();
uint8_t menuSelection();
uint8_t settingsMenuSelection();
bool clearCancelSelected();
void onMeasurement(float hb, const char *timestamp, bool valid, const char *status);
void setBatteryPlaceholders();
float soc();
float soh();
size_t logIndex();
void setLogIndex(size_t index);
}
#endif
