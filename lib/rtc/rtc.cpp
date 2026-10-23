
#include "../../lib/rtc/rtc.h"
#include "../max30102/../../include/config.h"

#include <Wire.h>
#include <RTClib.h>
#include <stdio.h>

namespace RTC
{
    static RTC_DS3231 rtc;
    static bool initialized = false;
    static DateTime cachedTime(2000, 1, 1, 0, 0, 0);

    
bool begin()
{
    // Reuse the I2C bus already initialized by the application.
    // Do not call Wire.setSDA() or Wire.setSCL() here.

    if (!rtc.begin(&Wire))
    {
        initialized = false;
        return false;
    }

    initialized = true;

    if (rtc.lostPower())
    {
        // Initialize RTC from the sketch compilation time.
        rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    }

    cachedTime = rtc.now();

    return true;
}


    bool isAvailable()
    {
        return initialized;
    }

    bool update()
    {
        if (!initialized)
            return false;

        cachedTime = rtc.now();
        return true;
    }

    bool getDateTime(DateTimeData &dateTime)
    {
        if (!update())
            return false;

        dateTime.year   = cachedTime.year();
        dateTime.month  = cachedTime.month();
        dateTime.day    = cachedTime.day();
        dateTime.hour   = cachedTime.hour();
        dateTime.minute = cachedTime.minute();
        dateTime.second = cachedTime.second();

        return true;
    }

    bool getTimestamp(char *buffer, size_t bufferSize)
    {
        if (buffer == nullptr || bufferSize < 20)
            return false;

        DateTimeData dt;

        if (!getDateTime(dt))
            return false;

        snprintf(
            buffer,
            bufferSize,
            "%04u-%02u-%02u %02u:%02u:%02u",
            dt.year,
            dt.month,
            dt.day,
            dt.hour,
            dt.minute,
            dt.second
        );

        return true;
    }

    bool setDateTime(
        uint16_t year,
        uint8_t month,
        uint8_t day,
        uint8_t hour,
        uint8_t minute,
        uint8_t second
    )
    {
        if (!initialized)
            return false;

        if (year < 2000 || year > 2099 ||
            month < 1 || month > 12 ||
            day < 1 || day > 31 ||
            hour > 23 || minute > 59 || second > 59)
        {
            return false;
        }

        rtc.adjust(
            DateTime(year, month, day, hour, minute, second)
        );

        cachedTime = rtc.now();
        return true;
    }
}
