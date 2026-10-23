
#ifndef RTC_H
#define RTC_H

#include <Arduino.h>

namespace RTC
{
    struct DateTimeData
    {
        uint16_t year;
        uint8_t month;
        uint8_t day;
        uint8_t hour;
        uint8_t minute;
        uint8_t second;
    };

    bool begin();
    bool isAvailable();
    bool update();

    bool getDateTime(DateTimeData &dateTime);

    bool getTimestamp(char *buffer, size_t bufferSize);

    bool setDateTime(
        uint16_t year,
        uint8_t month,
        uint8_t day,
        uint8_t hour,
        uint8_t minute,
        uint8_t second
    );
}

#endif
