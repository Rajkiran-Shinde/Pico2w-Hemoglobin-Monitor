
#include <Arduino.h>

#include "max30102.h"
#include "processing.h"
#include "hemoglobin.h"
#include "rtc.h"
#include "../include/config.h"

// Timing
static uint32_t lastEstimateMs = 0;
static uint32_t lastSequence = 0;
static bool samplingIntegrityValid = true;
static bool rtcAvailable = false;

void setup()
{
    Serial.begin(SERIAL_BAUD_RATE);
    delay(1000);

    Serial.println();
    Serial.println("================================");
    Serial.println(" Pico 2W Hemoglobin Monitor");
    Serial.println("================================");

    // Initialize the processing and Hb modules.
    Processing::begin();
    Hemoglobin::begin();

    // Initialize MAX30102 first.
    // Its driver initializes the shared I2C bus.
    if (!MAX30102::begin())
    {
        Serial.println("ERROR: MAX30102 initialization failed!");

        while (true)
        {
            delay(1000);
        }
    }

    Serial.println("MAX30102 initialized successfully.");

    // Initialize RTC using the same I2C bus.
    // RTC::begin() must not reconfigure Wire pins.
    rtcAvailable = RTC::begin();

    if (rtcAvailable)
    {
        Serial.println("RTC initialized successfully.");

        char timestamp[24];

        if (RTC::getTimestamp(timestamp, sizeof(timestamp)))
        {
            Serial.print("RTC Date/Time: ");
            Serial.println(timestamp);
        }
    }
    else
    {
        Serial.println("WARNING: RTC not detected.");
        Serial.println("Check DS3231 wiring and battery.");
    }

    Serial.println();
    Serial.println("Waiting for valid readings...");
}

void loop()
{
    // Update MAX30102 acquisition.
    MAX30102::update();

    // Detect queue overflow.
    static uint32_t lastOverflowCount = 0;

    uint32_t overflowCount =
        MAX30102::getQueueOverflowCount();

    if (overflowCount != lastOverflowCount)
    {
        lastOverflowCount = overflowCount;

        Processing::reset();
        lastSequence = 0;
        samplingIntegrityValid = false;

        Serial.println(
            "WARNING: Sample queue overflow. "
            "Rebuilding signal window."
        );
    }

    // Process all available sensor samples.
    MAX30102::RawSample sample;

    while (MAX30102::readSample(sample))
    {
        // Detect missing samples.
        if (lastSequence != 0 &&
            sample.sequence != lastSequence + 1)
        {
            Processing::reset();
            samplingIntegrityValid = false;

            Serial.println(
                "WARNING: Sample sequence gap detected."
            );
        }

        lastSequence = sample.sequence;

        Processing::addSample(sample.red, sample.ir);

        if (Processing::getSampleCount() ==
            Processing::WINDOW_SIZE)
        {
            samplingIntegrityValid = true;
        }
    }

    // Estimate Hb once per second when the signal is valid.
    if (samplingIntegrityValid &&
        Processing::isWindowReady() &&
        millis() - lastEstimateMs >= 1000)
    {
        lastEstimateMs = millis();

        Processing::Features features;

        if (Processing::calculateFeatures(features) &&
            features.signalValid)
        {
            Hemoglobin::Result result;

            Hemoglobin::Status status =
                Hemoglobin::estimate(features, result);

            if (status ==
                    Hemoglobin::Status::ESTIMATE_AVAILABLE &&
                result.available)
            {
                char timestamp[24];

                Serial.println();
                Serial.println("========== MEASUREMENT ==========");

                // Print RTC timestamp.
                if (rtcAvailable &&
                    RTC::getTimestamp(
                        timestamp,
                        sizeof(timestamp)))
                {
                    Serial.print("Date & Time : ");
                    Serial.println(timestamp);
                }
                else
                {
                    Serial.println(
                        "Date & Time : RTC unavailable"
                    );
                }

                // Print hemoglobin estimate.
                Serial.print("Hemoglobin  : ");
                Serial.print(result.valueGdL, 2);
                Serial.println(" g/dL");

                Serial.println("=================================");
            }
        }
    }
}
