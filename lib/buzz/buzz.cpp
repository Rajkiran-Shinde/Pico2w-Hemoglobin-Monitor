#include "buzz.h"
#include "../../include/config.h"

namespace Buzz {
struct Note {
    uint16_t frequency;
    uint16_t durationMs;
    uint16_t gapMs;
};

enum class Pattern : uint8_t { NONE, STARTUP, BUTTON, WARNING, CRITICAL };

static const Note startupNotes[] = {
    {523, 110, 35}, {659, 110, 35}, {784, 150, 45}, {1047, 220, 0}
};
static const Note buttonNotes[] = {
    {1800, 35, 0}
};
static const Note warningNotes[] = {
    {1000, 100, 70}, {800, 140, 0}
};
static const Note criticalNotes[] = {
    {1400, 180, 70}, {900, 180, 70}, {1400, 180, 70}, {900, 250, 0}
};

static Pattern activePattern = Pattern::NONE;
static uint8_t noteIndex = 0;
static uint32_t phaseStartedAt = 0;
static bool inGap = false;

static const Note* notesFor(Pattern p, uint8_t &count) {
    switch (p) {
        case Pattern::STARTUP:
            count = sizeof(startupNotes) / sizeof(startupNotes[0]); return startupNotes;
        case Pattern::BUTTON:
            count = sizeof(buttonNotes) / sizeof(buttonNotes[0]); return buttonNotes;
        case Pattern::WARNING:
            count = sizeof(warningNotes) / sizeof(warningNotes[0]); return warningNotes;
        case Pattern::CRITICAL:
            count = sizeof(criticalNotes) / sizeof(criticalNotes[0]); return criticalNotes;
        default:
            count = 0; return nullptr;
    }
}

static void startPattern(Pattern p) {
    activePattern = p;
    noteIndex = 0;
    inGap = false;
    phaseStartedAt = millis();
    uint8_t count = 0;
    const Note *notes = notesFor(activePattern, count);
    if (!notes || count == 0) {
        activePattern = Pattern::NONE;
        noTone(PIN_BUZZER);
        return;
    }
    if (notes[0].frequency) tone(PIN_BUZZER, notes[0].frequency);
    else noTone(PIN_BUZZER);
}

void begin() {
    pinMode(PIN_BUZZER, OUTPUT);
    noTone(PIN_BUZZER);
    activePattern = Pattern::NONE;
}

void playStartupTune() { startPattern(Pattern::STARTUP); }
void playButtonBeep() {
    // Keep a critical/warning/startup pattern from being interrupted by UI clicks.
    if (activePattern == Pattern::NONE || activePattern == Pattern::BUTTON)
        startPattern(Pattern::BUTTON);
}
void playWarning() {
    if (activePattern != Pattern::CRITICAL) startPattern(Pattern::WARNING);
}
void playCritical() { startPattern(Pattern::CRITICAL); }

void stop() {
    activePattern = Pattern::NONE;
    noTone(PIN_BUZZER);
}

bool isPlaying() { return activePattern != Pattern::NONE; }

void update() {
    if (activePattern == Pattern::NONE) return;

    uint8_t count = 0;
    const Note *notes = notesFor(activePattern, count);
    if (!notes || noteIndex >= count) {
        stop();
        return;
    }

    const uint32_t now = millis();
    const uint16_t phaseDuration = inGap ? notes[noteIndex].gapMs
                                         : notes[noteIndex].durationMs;
    if (now - phaseStartedAt < phaseDuration) return;

    phaseStartedAt = now;
    if (!inGap && notes[noteIndex].gapMs > 0) {
        noTone(PIN_BUZZER);
        inGap = true;
        return;
    }

    inGap = false;
    ++noteIndex;
    if (noteIndex >= count) {
        stop();
        return;
    }
    if (notes[noteIndex].frequency) tone(PIN_BUZZER, notes[noteIndex].frequency);
    else noTone(PIN_BUZZER);
}
}
