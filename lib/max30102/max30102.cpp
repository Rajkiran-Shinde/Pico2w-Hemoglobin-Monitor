
#include "max30102.h"
#include "../../include/config.h"

#include <MAX30105.h>
#include <math.h>

// ============================================================
// MAX30102 DRIVER + SOFTWARE SAMPLE QUEUE
// Raspberry Pi Pico 2W / Arduino Framework
// ============================================================

namespace
{
    MAX30105 sensor;
    bool sensorInitialized = false;

    MAX30102::SensorData latestData = {};

    // --------------------------------------------------------
    // SOFTWARE RAW-SAMPLE QUEUE
    // --------------------------------------------------------

    constexpr uint16_t QUEUE_SIZE = 128;

    MAX30102::RawSample sampleQueue[QUEUE_SIZE] = {};

    uint16_t queueHead = 0;
    uint16_t queueTail = 0;
    uint16_t queueCount = 0;

    uint32_t sampleSequence = 0;
    uint32_t queueOverflowCount = 0;

    uint32_t totalSamplesAcquired = 0;

    // --------------------------------------------------------
    // EXISTING MOVING-AVERAGE FILTER
    // --------------------------------------------------------

    constexpr uint8_t FILTER_SIZE = 8;

    uint32_t redBuffer[FILTER_SIZE] = {};
    uint32_t irBuffer[FILTER_SIZE] = {};

    uint8_t bufferIndex = 0;
    uint8_t samplesInBuffer = 0;

    uint64_t redSum = 0;
    uint64_t irSum = 0;

    // --------------------------------------------------------
    // DC BASELINE TRACKING
    // --------------------------------------------------------

    constexpr float DC_ALPHA = 0.05f;

    float redDC = 0.0f;
    float irDC = 0.0f;

    bool dcInitialized = false;

    // Tune after observing raw IR values on your hardware.
    constexpr uint32_t FINGER_THRESHOLD = 50000;

    // --------------------------------------------------------
    // SOFTWARE QUEUE OPERATIONS
    // --------------------------------------------------------

    void enqueueSample(uint32_t red, uint32_t ir)
    {
        // Drop the oldest queued sample if the queue is full.
        if (queueCount >= QUEUE_SIZE)
        {
            queueTail = (queueTail + 1U) % QUEUE_SIZE;
            queueCount--;

            queueOverflowCount++;
        }

        MAX30102::RawSample &slot = sampleQueue[queueHead];

        slot.red = red;
        slot.ir = ir;
        slot.sequence = sampleSequence++;

        totalSamplesAcquired++;

        queueHead = (queueHead + 1U) % QUEUE_SIZE;
        queueCount++;
    }

    // --------------------------------------------------------
    // PERFUSION INDEX
    // --------------------------------------------------------

    float calculatePI(float ac, float dc)
    {
        if (dc <= 0.0f)
        {
            return 0.0f;
        }

        return (fabsf(ac) / dc) * 100.0f;
    }

    // --------------------------------------------------------
    // HEURISTIC QUALITY SCORE
    // NOTE: Not a clinical validity measurement.
    // --------------------------------------------------------

    uint8_t calculateSignalQuality(
        float irAC,
        float irDC,
        bool fingerDetected)
    {
        if (!fingerDetected || irDC <= 0.0f)
        {
            return 0;
        }

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

    // --------------------------------------------------------
    // EXISTING FILTER AND SENSOR-DATA PROCESSING
    // --------------------------------------------------------

    void processSample(uint32_t red, uint32_t ir)
    {
        // Remove oldest samples from rolling sums.
        if (samplesInBuffer == FILTER_SIZE)
        {
            redSum -= redBuffer[bufferIndex];
            irSum -= irBuffer[bufferIndex];
        }
        else
        {
            samplesInBuffer++;
        }

        // Insert current sample.
        redBuffer[bufferIndex] = red;
        irBuffer[bufferIndex] = ir;

        redSum += red;
        irSum += ir;

        bufferIndex = (bufferIndex + 1U) % FILTER_SIZE;

        // Moving-average filter.
        uint32_t filteredRed =
            static_cast<uint32_t>(redSum / samplesInBuffer);

        uint32_t filteredIR =
            static_cast<uint32_t>(irSum / samplesInBuffer);

        // Initialize baseline.
        if (!dcInitialized)
        {
            redDC = static_cast<float>(filteredRed);
            irDC = static_cast<float>(filteredIR);

            dcInitialized = true;
        }

        // Track slowly changing DC baseline.
        redDC += DC_ALPHA * (filteredRed - redDC);
        irDC += DC_ALPHA * (filteredIR - irDC);

        // Pulsatile residual.
        float redAC = static_cast<float>(filteredRed) - redDC;
        float irAC = static_cast<float>(filteredIR) - irDC;

        bool fingerDetected =
            (filteredIR > FINGER_THRESHOLD);

        latestData.red = red;
        latestData.ir = ir;

        latestData.filteredRed = filteredRed;
        latestData.filteredIR = filteredIR;

        latestData.redAC = redAC;
        latestData.irAC = irAC;

        latestData.redPerfusionIndex =
            calculatePI(redAC, redDC);

        latestData.irPerfusionIndex =
            calculatePI(irAC, irDC);

        latestData.fingerDetected = fingerDetected;

        latestData.signalQuality = calculateSignalQuality(
            irAC,
            irDC,
            fingerDetected
        );

        latestData.signalValid =
            fingerDetected &&
            samplesInBuffer == FILTER_SIZE &&
            latestData.signalQuality >= 40;
    }

} // End anonymous namespace


// ============================================================
// INITIALIZATION
// ============================================================

bool MAX30102::begin(TwoWire &wire)
{
    sensorInitialized = false;

    if (!sensor.begin(wire, I2C_SPEED_FAST))
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

    // Reset software queue.
    queueHead = 0;
    queueTail = 0;
    queueCount = 0;

    sampleSequence = 0;
    queueOverflowCount = 0;

    // Reset moving-average filter.
    bufferIndex = 0;
    samplesInBuffer = 0;

    redSum = 0;
    irSum = 0;

    redDC = 0.0f;
    irDC = 0.0f;

    totalSamplesAcquired = 0;

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
// INITIALIZATION STATUS
// ============================================================

bool MAX30102::isInitialized()
{
    return sensorInitialized;
}


// ============================================================
// SENSOR ACQUISITION
// ============================================================

void MAX30102::update()
{
    if (!sensorInitialized)
    {
        return;
    }

    // Refresh the library's view of the hardware FIFO.
    sensor.check();

    // Drain all samples currently available.
    while (sensor.available())
    {
        uint32_t red = sensor.getRed();
        uint32_t ir = sensor.getIR();

        // Preserve each raw sample for the main processor.
        enqueueSample(red, ir);

        // Maintain backward-compatible SensorData output.
        processSample(red, ir);

        // Advance to the next FIFO entry.
        sensor.nextSample();
    }
}


// ============================================================
// RAW-SAMPLE QUEUE API
// ============================================================

bool MAX30102::readSample(RawSample &sample)
{
    if (queueCount == 0)
    {
        return false;
    }

    sample = sampleQueue[queueTail];

    queueTail = (queueTail + 1U) % QUEUE_SIZE;
    queueCount--;

    return true;
}


uint32_t MAX30102::getQueueOverflowCount()
{
    return queueOverflowCount;
}

uint32_t MAX30102::getTotalSamplesAcquired()
{
    return totalSamplesAcquired;
}

uint32_t MAX30102::getQueueDepth()
{
    return queueCount;
}


// ============================================================
// EXISTING SENSOR-DATA API
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
    // Disabled until a validated calibration model exists.
    hbGdL = 0.0f;

    return false;
}
