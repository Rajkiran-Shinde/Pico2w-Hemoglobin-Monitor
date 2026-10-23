#ifndef STORAGE_H
#define STORAGE_H
#include <Arduino.h>
namespace Storage {
struct Record { char timestamp[20]; float hb; char status[24]; };
bool begin();
bool isAvailable();
bool append(float hb, const char *timestamp, const char *status = "OK");
size_t count();
bool read(size_t index, Record &record);
bool clear();
}
#endif
