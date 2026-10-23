#include "buttons.h"
#include "../../include/config.h"
namespace Buttons {
static bool b1StableHigh = true, b1LastReadHigh = true, b2StableHigh = true, b2LastReadHigh = true;
static uint32_t b1ChangedAt = 0, b2ChangedAt = 0, b1PressedAt = 0;
static bool b1LongSent = false;
void begin() {
  pinMode(PIN_BUTTON_SELECT, INPUT_PULLUP); // B1: select/back
  pinMode(PIN_BUTTON_MODE, INPUT_PULLUP);   // B2: navigation/toggle
}
Event poll() {
  const uint32_t now = millis();
  bool b1 = digitalRead(PIN_BUTTON_SELECT), b2 = digitalRead(PIN_BUTTON_MODE);
  if (b1 != b1LastReadHigh) { b1LastReadHigh = b1; b1ChangedAt = now; }
  if (b2 != b2LastReadHigh) { b2LastReadHigh = b2; b2ChangedAt = now; }
  if (now - b1ChangedAt >= BUTTON_DEBOUNCE_MS && b1 != b1StableHigh) {
    b1StableHigh = b1;
    if (!b1StableHigh) { b1PressedAt = now; b1LongSent = false; }
    else if (!b1LongSent) return Event::B1_SHORT;
  }
  if (!b1StableHigh && !b1LongSent && now - b1PressedAt >= 800) {
    b1LongSent = true; return Event::B1_LONG;
  }
  if (now - b2ChangedAt >= BUTTON_DEBOUNCE_MS && b2 != b2StableHigh) {
    b2StableHigh = b2;
    if (!b2StableHigh) return Event::B2_PRESS;
  }
  return Event::NONE;
}
}
