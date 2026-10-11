
#ifndef PROCESSING_H
#define PROCESSING_H

#include <Arduino.h>

namespace Processing
{
    // Number of samples in one analysis window.
    constexpr uint16_t WINDOW_SIZE = 500;

    struct Features
    {
        // DC components: mean optical intensity
        float redDC;
        float irDC;

        // AC components: RMS variation around the mean
        float redAC_RMS;
        float irAC_RMS;

        // Normalized pulsatile amplitudes (%)
        float redNormalizedAC;
        float irNormalizedAC;

        // Optical ratio; not an Hb concentration equation
        float opticalRatio;

        // Peak-to-peak variation
        uint32_t redPeakToPeak;
        uint32_t irPeakToPeak;

        // Input signal-quality score (0-100)
        uint8_t signalQuality;

        bool fingerDetected;
        bool windowReady;
        bool signalValid;
    };

    // Clear all samples and calculated features.
    void begin();

    // Add one sample from the MAX30102 driver.
    void addSample(
        uint32_t red,
        uint32_t ir,
        bool fingerDetected,
        uint8_t sensorQuality
    );

    // Returns true after a complete window is collected.
    bool isWindowReady();

    // Calculate features from the current window.
    bool calculateFeatures(Features &features);

    // Clear the window and start collecting again.
    void reset();

    // Number of samples currently collected.
    uint16_t getSampleCount();
}

#endif // PROCESSING_H
