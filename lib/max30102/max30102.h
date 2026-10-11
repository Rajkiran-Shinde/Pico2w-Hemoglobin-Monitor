
#ifndef MAX30102_H
#define MAX30102_H

#include <Arduino.h>
#include <Wire.h>

namespace MAX30102
{
    struct SensorData
    {
        // Latest raw samples
        uint32_t red;
        uint32_t ir;

        // Moving-average filtered samples
        uint32_t filteredRed;
        uint32_t filteredIR;

        // AC component after DC baseline removal
        float redAC;
        float irAC;

        // Approximate pulsatile signal strength
        float redPerfusionIndex;
        float irPerfusionIndex;

        // Heuristic signal quality: 0 to 100
        uint8_t signalQuality;

        bool fingerDetected;
        bool signalValid;
    };

    bool begin(TwoWire &wire = Wire);
    bool isInitialized();

    void update();
    bool readSensor(SensorData &data);

    bool isFingerDetected();

    uint32_t getRed();
    uint32_t getIR();

    SensorData getLatestData();

    // Returns false until a validated Hb calibration model
    // has been implemented.
    bool getHemoglobinEstimate(float &hbGdL);
}

#endif // MAX30102_H
