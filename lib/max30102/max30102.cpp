
#include "max30102.h"
#include "../../include/config.h"
#include <MAX30105.h>
#include <math.h>

// ============================================================
// MAX30102 DRIVER WITH BASIC SIGNAL PROCESSING
// ============================================================

namespace
{
    MAX30105 sensor;
    bool sensorInitialized = false;

    MAX30102::SensorData latestData = {};

    // Moving-average filter
    constexpr uint8_t FILTER_SIZE = 8;

    uint32_t redBuffer[FILTER_SIZE] = {};
    uint32_t irBuffer[FILTER_SIZE] = {};

    uint8_t bufferIndex = 0;
    uint8_t samplesInBuffer = 0;

    uint64_t redSum = 0;
    uint64_t irSum = 0;

    // Exponential DC baseline tracking
    constexpr float DC_ALPHA = 0.05f;

    float redDC = 0.0f;
    float irDC = 0.0f;

    bool dcInitialized = false;

    // Tune for your sensor and finger placement.
    constexpr uint32_t FINGER_THRESHOLD = 50000;

    float calculatePI(float ac, float dc)
    {
        if (dc <= 0.0f)
        {
            return 0.0f;
        }

        return (fabsf(ac) / dc) * 100.0f;
    }

    uint8_t calculateSignalQuality(
        uint32_t ir,
        float irAC,
        float irDC,
        bool fingerDetected)
    {
        if (!fingerDetected || irDC <= 0.0f)
        {
            return 0;
        }

        // Heuristic quality score, not a medical validation.
        float pi = calculatePI(irAC, irDC);

        if (pi < 0.02f)
        {
            return 20;
        }

        if (pi < 0.05f)
        {
            return 40;
        }

        if (pi < 0.10f)
        {
            return 60;
        }

        if (pi < 2.0f)
        {
            return 80;
        }

        return 70;
    }

    void processSample(uint32_t red, uint32_t ir)
    {
        // Remove the oldest sample from the rolling sums.
        if (samplesInBuffer == FILTER_SIZE)
        {
            redSum -= redBuffer[bufferIndex];
            irSum -= irBuffer[bufferIndex];
        }
        else
        {
            samplesInBuffer++;
        }

        // Add the new sample.
        redBuffer[bufferIndex] = red;
        irBuffer[bufferIndex] = ir;

        redSum += red;
        irSum += ir;

        bufferIndex = (bufferIndex + 1) % FILTER_SIZE;

        // Moving-average filtering.
        uint32_t filteredRed =
            static_cast<uint32_t>(redSum / samplesInBuffer);

        uint32_t filteredIR =
            static_cast<uint32_t>(irSum / samplesInBuffer);

        // Initialize DC baselines on the first sample.
        if (!dcInitialized)
        {
            redDC = static_cast<float>(filteredRed);
            irDC = static_cast<float>(filteredIR);
            dcInitialized = true;
        }

        // Exponential moving average estimates the slowly
        // changing DC component.
        redDC += DC_ALPHA * (filteredRed - redDC);
        irDC += DC_ALPHA * (filteredIR - irDC);

        // AC component is the difference from the DC baseline.
        float redAC = filteredRed - redDC;
        float irAC = filteredIR - irDC;

        bool fingerDetected = (filteredIR > FINGER_THRESHOLD);

        latestData.red = red;
        latestData.ir = ir;

        latestData.filteredRed = filteredRed;
        latestData.filteredIR = filteredIR;

        latestData.redAC = redAC;
        latestData.irAC = irAC;

        latestData.redPerfusionIndex = calculatePI(redAC, redDC);
        latestData.irPerfusionIndex = calculatePI(irAC, irDC);

        latestData.fingerDetected = fingerDetected;

        latestData.signalQuality = calculateSignalQuality(
            filteredIR,
            irAC,
            irDC,
            fingerDetected
        );

        latestData.signalValid =
            fingerDetected &&
            samplesInBuffer == FILTER_SIZE &&
            latestData.signalQuality >= 40;
    }
}

// ============================================================
// INITIALIZATION
// ============================================================

bool MAX30102::begin(TwoWire &wire)
{
    sensorInitialized = false;

    if (!sensor.begin(wire, I2C_SPEED_STANDARD))
    {
        return false;
    }

    sensor.setup(
        MAX30102_LED_BRIGHTNESS,
        MAX30102_SAMPLE_AVERAGE,
        MAX30102_LED_MODE,
        MAX30102_SAMPLE_RATE,
        MAX30102_PULSE_WIDTH,
        MAX30102_ADC_RANGE
    );

    // Clear filter state.
    bufferIndex = 0;
    samplesInBuffer = 0;

    redSum = 0;
    irSum = 0;

    redDC = 0.0f;
    irDC = 0.0f;
    dcInitialized = false;

    latestData = {};

    for (uint8_t i = 0; i < FILTER_SIZE; i++)
    {
        redBuffer[i] = 0;
        irBuffer[i] = 0;
    }

    sensorInitialized = true;
    return true;
}

// ============================================================
// STATUS
// ============================================================

bool MAX30102::isInitialized()
{
    return sensorInitialized;
}

// ============================================================
// ACQUIRE AND PROCESS SAMPLES
// ============================================================

void MAX30102::update()
{
    if (!sensorInitialized)
    {
        return;
    }

    sensor.check();

    // Process every sample currently available in the FIFO.
    while (sensor.available())
    {
        uint32_t red = sensor.getRed();
        uint32_t ir = sensor.getIR();

        processSample(red, ir);

        sensor.nextSample();
    }
}

// ============================================================
// PUBLIC DATA API
// ============================================================

bool MAX30102::readSensor(SensorData &data)
{
    if (!sensorInitialized)
    {
        return false;
    }

    update();
    data = latestData;

    return true;
}

bool MAX30102::isFingerDetected()
{
    return latestData.fingerDetected;
}

uint32_t MAX30102::getRed()
{
    return latestData.red;
}

uint32_t MAX30102::getIR()
{
    return latestData.ir;
}

MAX30102::SensorData MAX30102::getLatestData()
{
    return latestData;
}

// ============================================================
// HEMOGLOBIN ESTIMATION INTERFACE
// ============================================================

bool MAX30102::getHemoglobinEstimate(float &hbGdL)
{
    // Deliberately disabled until a validated calibration
    // model and its coefficients are available.
    hbGdL = 0.0f;
    return false;
}
