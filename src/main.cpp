#include <Arduino.h>
#include "max30102.h"
#include "processing.h"
#include "Hemoglobin.h"
#include "../include/config.h"

static uint32_t lastEstimateMs = 0;
static uint32_t lastSequence = 0;
static bool samplingIntegrityValid = true;

void setup()
{
    Serial.begin(SERIAL_BAUD_RATE);

    Processing::begin();
    Hemoglobin::begin();

    if (!MAX30102::begin())
    {
        Serial.println("Sensor initialization failed");
        while (true)
        {
            delay(1000);
        }
    }
}

void loop()
{
    MAX30102::update();

    // Detect lost samples or queue overflow.
    static uint32_t lastOverflowCount = 0;

    uint32_t overflowCount =
        MAX30102::getQueueOverflowCount();

    if (overflowCount != lastOverflowCount)
    {
        lastOverflowCount = overflowCount;
        Processing::reset();
        lastSequence = 0;
        samplingIntegrityValid = false;
    }

    MAX30102::RawSample sample;

    while (MAX30102::readSample(sample))
    {
        if (lastSequence != 0 &&
            sample.sequence != lastSequence + 1)
        {
            Processing::reset();
            samplingIntegrityValid = false;
        }

        lastSequence = sample.sequence;

        Processing::addSample(sample.red, sample.ir);

        if (Processing::getSampleCount() ==
            Processing::WINDOW_SIZE)
        {
            samplingIntegrityValid = true;
        }
    }

    // Calculate and print once per second.
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

            if (status == Hemoglobin::Status::ESTIMATE_AVAILABLE &&
                result.available)
            {
                Serial.print("Experimental Hb: ");
                Serial.print(result.valueGdL, 2);
                Serial.println(" g/dL");
            }
        }
    }
}