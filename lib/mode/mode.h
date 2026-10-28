#ifndef MODE_H
#define MODE_H
#include <Arduino.h>
#include "../buttons/buttons.h"
#include "../storage/storage.h"

namespace Mode {
enum class Screen : uint8_t { HOME, HB_READY, MEASURE, LOGS, LOG_DETAIL, CLEAR_CONFIRM, MESSAGE };
enum class MeasureMode : uint8_t { SINGLE };
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
bool clearCancelSelected();
void onMeasurement(float hb, const char *timestamp, bool valid, const char *status);
size_t logIndex();
void setLogIndex(size_t index);
}
#endif
