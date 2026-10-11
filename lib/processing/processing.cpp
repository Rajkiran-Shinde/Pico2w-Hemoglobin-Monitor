
#include "processing.h"
#include <math.h>

namespace
{
    constexpr uint16_t N = Processing::WINDOW_SIZE;

    uint32_t redBuffer[N] = {};
    uint32_t irBuffer[N] = {};

    bool fingerBuffer[N] = {};
    uint8_t qualityBuffer[N] = {};

    uint16_t writeIndex = 0;
    uint16_t sampleCount = 0;

    bool windowReady = false;

    // ========================================================
    // Mean
    // ========================================================

    double calculateMean(const uint32_t *buffer)
    {
        double sum = 0.0;

        for (uint16_t i = 0; i < N; i++)
        {
            sum += buffer[i];
        }

        return sum / N;
    }

    // ========================================================
    // RMS AC variation around the mean
    // ========================================================

    double calculateAC_RMS(
        const uint32_t *buffer,
        double mean
    )
    {
        double sumSquares = 0.0;

        for (uint16_t i = 0; i < N; i++)
        {
            double difference = buffer[i] - mean;
            sumSquares += difference * difference;
        }

        return sqrt(sumSquares / N);
    }

    // ========================================================
    // Peak-to-peak variation
    // ========================================================

    uint32_t calculatePeakToPeak(const uint32_t *buffer)
    {
        uint32_t minimum = UINT32_MAX;
        uint32_t maximum = 0;

        for (uint16_t i = 0; i < N; i++)
        {
            if (buffer[i] < minimum)
            {
                minimum = buffer[i];
            }

            if (buffer[i] > maximum)
            {
                maximum = buffer[i];
            }
        }

        return maximum - minimum;
    }

    // ========================================================
    // Calculate average sensor quality and finger presence
    // ========================================================

    void calculateInputQuality(
        uint8_t &quality,
        bool &fingerDetected
    )
    {
        uint32_t qualitySum = 0;
        uint16_t fingerSamples = 0;

        for (uint16_t i = 0; i < N; i++)
        {
            qualitySum += qualityBuffer[i];

            if (fingerBuffer[i])
            {
                fingerSamples++;
            }
        }

        quality = static_cast<uint8_t>(qualitySum / N);

        // Require finger contact for at least 90% of the window.
        fingerDetected = (fingerSamples >= (N * 9U) / 10U);
    }
}

// ============================================================
// INITIALIZATION
// ============================================================

void Processing::begin()
{
    reset();
}

// ============================================================
// ADD ONE SAMPLE
// ============================================================

void Processing::addSample(
    uint32_t red,
    uint32_t ir,
    bool fingerDetected,
    uint8_t sensorQuality
)
{
    redBuffer[writeIndex] = red;
    irBuffer[writeIndex] = ir;

    fingerBuffer[writeIndex] = fingerDetected;

    if (sensorQuality > 100)
    {
        sensorQuality = 100;
    }

    qualityBuffer[writeIndex] = sensorQuality;

    writeIndex = (writeIndex + 1) % N;

    if (sampleCount < N)
    {
        sampleCount++;
    }

    windowReady = (sampleCount == N);
}

// ============================================================
// WINDOW STATUS
// ============================================================

bool Processing::isWindowReady()
{
    return windowReady;
}

uint16_t Processing::getSampleCount()
{
    return sampleCount;
}

// ============================================================
// FEATURE EXTRACTION
// ============================================================

bool Processing::calculateFeatures(Features &features)
{
    features = {};

    features.windowReady = windowReady;

    if (!windowReady)
    {
        return false;
    }

    // Calculate DC means.
    double redMean = calculateMean(redBuffer);
    double irMean = calculateMean(irBuffer);

    // Calculate AC variation.
    double redAC = calculateAC_RMS(redBuffer, redMean);
    double irAC = calculateAC_RMS(irBuffer, irMean);

    features.redDC = static_cast<float>(redMean);
    features.irDC = static_cast<float>(irMean);

    features.redAC_RMS = static_cast<float>(redAC);
    features.irAC_RMS = static_cast<float>(irAC);

    // Normalize AC variation by its corresponding DC level.
    if (redMean > 0.0)
    {
        features.redNormalizedAC =
            static_cast<float>((redAC / redMean) * 100.0);
    }

    if (irMean > 0.0)
    {
        features.irNormalizedAC =
            static_cast<float>((irAC / irMean) * 100.0);
    }

    features.redPeakToPeak = calculatePeakToPeak(redBuffer);
    features.irPeakToPeak = calculatePeakToPeak(irBuffer);

    // Ratio of normalized red and IR variations.
    double redRatio = (redMean > 0.0)
        ? redAC / redMean
        : 0.0;

    double irRatio = (irMean > 0.0)
        ? irAC / irMean
        : 0.0;

    if (irRatio > 1e-9)
    {
        features.opticalRatio =
            static_cast<float>(redRatio / irRatio);
    }

    // Aggregate sensor quality and finger contact.
    calculateInputQuality(
        features.signalQuality,
        features.fingerDetected
    );

    // Basic engineering gate, not a clinical validity test.
    features.signalValid =
        features.fingerDetected &&
        features.signalQuality >= 40 &&
        redMean > 0.0 &&
        irMean > 0.0 &&
        redAC > 0.0 &&
        irAC > 0.0;

    return true;
}

// ============================================================
// RESET
// ============================================================

void Processing::reset()
{
    for (uint16_t i = 0; i < N; i++)
    {
        redBuffer[i] = 0;
        irBuffer[i] = 0;

        fingerBuffer[i] = false;
        qualityBuffer[i] = 0;
    }

    writeIndex = 0;
    sampleCount = 0;
    windowReady = false;
}
