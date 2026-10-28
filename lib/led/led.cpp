#include "led.h"
#include "../../include/config.h"

namespace Led {
static State currentState = State::NORMAL;
static uint32_t lastBlinkMs = 0;
static bool blinkOn = true;

static void writeLeds(bool red, bool green, bool blue, bool yellow) {
    digitalWrite(PIN_LED_RED, red ? HIGH : LOW);
    digitalWrite(PIN_LED_GREEN, green ? HIGH : LOW);
    digitalWrite(PIN_LED_BLUE, blue ? HIGH : LOW);
    digitalWrite(PIN_LED_YELLOW, yellow ? HIGH : LOW);
}

void begin() {
    pinMode(PIN_LED_RED, OUTPUT);
    pinMode(PIN_LED_GREEN, OUTPUT);
    pinMode(PIN_LED_BLUE, OUTPUT);
    pinMode(PIN_LED_YELLOW, OUTPUT);
    currentState = State::NORMAL;
    lastBlinkMs = millis();
    blinkOn = true;
    writeLeds(false, false, false, false);
}

void setState(State next) {
    if (next != currentState) {
        currentState = next;
        lastBlinkMs = millis();
        blinkOn = true;
    }
}

State state() { return currentState; }

void update() {
    const uint32_t now = millis();

    // Steady green means ready. Blue pulses to indicate activity/result.
    // Yellow and red blink to distinguish warning from critical fault.
    switch (currentState) {
        case State::NORMAL:
            writeLeds(false, true, false, false);
            break;

        case State::ACTIVITY:
            if (now - lastBlinkMs >= 350) {
                lastBlinkMs = now;
                blinkOn = !blinkOn;
            }
            writeLeds(false, false, blinkOn, false);
            break;

        case State::WARNING:
            if (now - lastBlinkMs >= 250) {
                lastBlinkMs = now;
                blinkOn = !blinkOn;
            }
            writeLeds(false, false, false, blinkOn);
            break;

        case State::CRITICAL:
            if (now - lastBlinkMs >= 120) {
                lastBlinkMs = now;
                blinkOn = !blinkOn;
            }
            writeLeds(blinkOn, false, false, false);
            break;
    }
}
}
