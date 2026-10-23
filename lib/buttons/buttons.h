#ifndef BUTTONS_H
#define BUTTONS_H
#include <Arduino.h>
namespace Buttons {
enum class Event : uint8_t { NONE, B1_SHORT, B1_LONG, B2_PRESS };
void begin();
Event poll();
}
#endif
