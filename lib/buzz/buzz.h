#ifndef BUZZ_H
#define BUZZ_H

#include <Arduino.h>

namespace Buzz {
void begin();
void playStartupTune();
void playButtonBeep();
void playWarning();
void playCritical();
void stop();
void update();
bool isPlaying();
}

#endif
