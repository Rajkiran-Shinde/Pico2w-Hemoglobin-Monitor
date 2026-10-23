#include "mode.h"
#include <string.h>
namespace Mode {
static Screen current = Screen::HOME, previous = Screen::HOME;
static MeasureMode selectedMode = MeasureMode::SINGLE;
static uint8_t homeSelection = 0, settingsSelection = 0;
static size_t currentLog = 0;
static char msg[64] = "Ready";
static uint32_t msgUntil = 0;
static bool requestMeasure = false;
static bool singleDone = false;
static bool clearChoice = false;
static float batterySoc = 76.0f, batterySoh = 98.0f; // TODO: replace with INA219/fuel-gauge implementation.
void begin() {}
Screen screen() { if (current == Screen::MESSAGE && millis() >= msgUntil) current = previous; return current; }
MeasureMode measureMode() { return selectedMode; }
void setMessage(const char *message, uint32_t durationMs) { previous = current; strncpy(msg, message ? message : "Error", sizeof(msg)-1); msg[sizeof(msg)-1] = 0; msgUntil = millis() + durationMs; current = Screen::MESSAGE; }
const char *message() { return msg; }
bool measurementRequested() { return requestMeasure; }
bool measurementActive() { return screen() == Screen::MEASURE; }
bool singleMeasurementComplete() { return singleDone; }
uint8_t menuSelection() { return homeSelection; }
uint8_t settingsMenuSelection() { return settingsSelection; }
bool clearCancelSelected() { return clearChoice; }
void clearMeasurementRequest() { requestMeasure = false; }
void onMeasurement(float hb, const char *timestamp, bool valid, const char *status) {
  if (!valid) { setMessage(status ? status : "Signal invalid", 2200); return; }
  if (selectedMode == MeasureMode::SINGLE) {
    if (singleDone) return;
    if (!Storage::append(hb, timestamp, "OK")) { setMessage("Storage error", 2500); return; }
    singleDone = true;
    previous = Screen::MEASURE;
    setMessage("Reading saved", 1600);
  }
}
void setBatteryPlaceholders() {}
float soc() { return batterySoc; }
float soh() { return batterySoh; }
size_t logIndex() { return currentLog; }
void setLogIndex(size_t index) { currentLog = index; }
void handle(Buttons::Event e) {
  if (e == Buttons::Event::NONE) return;
  Screen s = screen();
  if (e == Buttons::Event::B1_LONG) {
    if (s == Screen::HOME) return;
    current = (s == Screen::CLEAR_CONFIRM) ? Screen::SETTINGS : Screen::HOME; return;
  }
  if (s == Screen::MESSAGE) { if (e == Buttons::Event::B1_SHORT) current = previous; return; }
  if (s == Screen::HOME) {
    if (e == Buttons::Event::B2_PRESS) homeSelection = (homeSelection + 1) % 4;
    else if (e == Buttons::Event::B1_SHORT) {
      if (homeSelection == 0) { selectedMode = MeasureMode::CONTINUOUS; singleDone = false; current = Screen::MEASURE; }
      else if (homeSelection == 1) { selectedMode = MeasureMode::SINGLE; singleDone = false; current = Screen::MEASURE; setMessage("Place finger on sensor", 2200); }
      else if (homeSelection == 2) { current = Screen::LOGS; currentLog = 0; }
      else { current = Screen::SETTINGS; settingsSelection = 0; }
    }
  } else if (s == Screen::MEASURE) {
    if (e == Buttons::Event::B1_SHORT) { current = Screen::HOME; }
    else if (e == Buttons::Event::B2_PRESS) { selectedMode = selectedMode == MeasureMode::SINGLE ? MeasureMode::CONTINUOUS : MeasureMode::SINGLE; }
  } else if (s == Screen::LOGS) {
    size_t n = Storage::count();
    if (e == Buttons::Event::B2_PRESS && n) currentLog = (currentLog + 1) % n;
    else if (e == Buttons::Event::B1_SHORT) current = Screen::HOME;
  } else if (s == Screen::SETTINGS) {
    if (e == Buttons::Event::B2_PRESS) settingsSelection = (settingsSelection + 1) % 3;
    else if (e == Buttons::Event::B1_SHORT) { if (settingsSelection == 2) { current = Screen::CLEAR_CONFIRM; clearChoice = false; } else setMessage("Battery values are demo", 1800); }
  } else if (s == Screen::CLEAR_CONFIRM) {
    if (e == Buttons::Event::B2_PRESS) clearChoice = !clearChoice;
    else if (e == Buttons::Event::B1_SHORT) {
      if (!clearChoice) { current = Screen::SETTINGS; setMessage(Storage::clear() ? "Logs cleared" : "Clear failed", 2000); }
      else current = Screen::SETTINGS;
    }
  }
}
}
