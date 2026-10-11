
#ifndef PROCESSING_H
#define PROCESSING_H

#include <Arduino.h>

namespace Processing
{
    constexpr uint16_t WINDOW_SIZE = 500;

    struct Features
    {
        float redDC = 0.0f;
        float irDC = 0.0f;

        float redAC_RMS = 0.0f;
        float irAC_RMS = 0.0f;

        float redNormalizedAC = 0.0f;
        float irNormalizedAC = 0.0f;

        float opticalRatio = 0.0f;

        uint32_t redPeakToPeak = 0;
        uint32_t irPeakToPeak = 0;

        uint8_t signalQuality = 0;
        uint8_t contactPercent = 0;
        uint8_t clippedPercent = 0;

        bool fingerDetected = false;
        bool windowReady = false;
        bool signalValid = false;
    };

    
struct FilteredSample
{
    uint32_t rawRed = 0;
    uint32_t rawIR = 0;

    float redDC = 0.0f;
    float irDC = 0.0f;

    float redAC = 0.0f;
    float irAC = 0.0f;

    bool ready = false;
};

bool getLatestFilteredSample(FilteredSample &sample);


    void begin();

    // Ingest one raw red/IR sample.
    void addSample(uint32_t red, uint32_t ir);

    bool isWindowReady();
    uint16_t getSampleCount();

    // Calculates features and window-level validation.
    bool calculateFeatures(Features &features);

    void reset();
}

#endif
