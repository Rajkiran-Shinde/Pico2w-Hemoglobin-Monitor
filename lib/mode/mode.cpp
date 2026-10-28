#include "mode.h"
#include <string.h>

namespace Mode {
static Screen current = Screen::HOME;
static Screen previous = Screen::HOME;
static uint8_t homeSelection = 0;
static size_t currentLog = 0;
static char msg[64] = "Ready";
static uint32_t msgUntil = 0;
static bool requestMeasure = false;
static bool singleDone = false;
static bool clearChoice = false; // false = YES, true = NO

void begin() {}
Screen screen() {
    if (current == Screen::MESSAGE && (int32_t)(millis() - msgUntil) >= 0)
        current = previous;
    return current;
}
MeasureMode measureMode() { return MeasureMode::SINGLE; }
void setMessage(const char *message, uint32_t durationMs) {
    previous = current;
    strncpy(msg, message ? message : "Error", sizeof(msg) - 1);
    msg[sizeof(msg) - 1] = '\0';
    msgUntil = millis() + durationMs;
    current = Screen::MESSAGE;
}
const char *message() { return msg; }
bool measurementRequested() { return requestMeasure; }
void clearMeasurementRequest() { requestMeasure = false; }
bool measurementActive() { return screen() == Screen::MEASURE; }
bool singleMeasurementComplete() { return singleDone; }
uint8_t menuSelection() { return homeSelection; }
bool clearCancelSelected() { return clearChoice; }
size_t logIndex() { return currentLog; }
void setLogIndex(size_t index) { currentLog = index; }

void onMeasurement(float hb, const char *timestamp, bool valid, const char *status) {
    if (!valid) {
        setMessage(status ? status : "Signal invalid", 2200);
        return;
    }
    if (singleDone) return;

    const char *recordStatus =
        (status && strcmp(status, "EXPERIMENTAL") == 0) ? "EXPERIMENTAL" : "OK";

    if (!Storage::append(hb, timestamp ? timestamp : "RTC unavailable", recordStatus)) {
        setMessage("Save failed - storage", 2500);
        return;
    }

    singleDone = true;
    setMessage("Reading saved", 1800);
}

void handle(Buttons::Event e) {
    if (e == Buttons::Event::NONE) return;
    Screen s = screen();

    if (s == Screen::MESSAGE) {
        if (e == Buttons::Event::B1_SHORT) current = previous;
        return;
    }

    if (e == Buttons::Event::B1_LONG) {
        if (s == Screen::CLEAR_CONFIRM) current = Screen::HOME;
        else if (s != Screen::HOME) current = Screen::HOME;
        return;
    }

    if (s == Screen::HOME) {
        if (e == Buttons::Event::B2_PRESS) {
            homeSelection = (homeSelection + 1U) % 3U;
        } else if (e == Buttons::Event::B1_SHORT) {
            if (homeSelection == 0) {
                singleDone = false;
                current = Screen::HB_READY;
            } else if (homeSelection == 1) {
                currentLog = 0;
                current = Screen::LOGS;
            } else {
                clearChoice = false;
                current = Screen::CLEAR_CONFIRM;
            }
        }
    } else if (s == Screen::HB_READY) {
        if (e == Buttons::Event::B1_SHORT) {
            singleDone = false;
            requestMeasure = true;
            current = Screen::MEASURE;
        } else if (e == Buttons::Event::B2_PRESS) {
            current = Screen::HOME;
        }
    } else if (s == Screen::MEASURE) {
        // B1 short is intentionally ignored during acquisition. Hold B1 to exit.
    } else if (s == Screen::LOGS) {
        const size_t n = Storage::count();
        if (e == Buttons::Event::B2_PRESS && n > 0) {
            currentLog = (currentLog + 1U) % n;
        } else if (e == Buttons::Event::B1_SHORT) {
            if (n > 0) current = Screen::LOG_DETAIL;
            else current = Screen::HOME;
        }
    } else if (s == Screen::LOG_DETAIL) {
        const size_t n = Storage::count();
        if (e == Buttons::Event::B1_SHORT) {
            current = Screen::LOGS;
        } else if (e == Buttons::Event::B2_PRESS && n > 0) {
            currentLog = (currentLog + 1U) % n;
        }
    } else if (s == Screen::CLEAR_CONFIRM) {
        if (e == Buttons::Event::B2_PRESS) {
            clearChoice = !clearChoice;
        } else if (e == Buttons::Event::B1_SHORT) {
            if (!clearChoice) {
                const bool ok = Storage::clear();
                current = Screen::HOME;
                setMessage(ok ? "Logs cleared" : "Clear failed", 2000);
            } else {
                current = Screen::HOME;
            }
        }
    }
}
}
