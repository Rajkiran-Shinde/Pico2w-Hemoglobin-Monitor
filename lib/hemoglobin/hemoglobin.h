
#ifndef HEMOGLOBIN_H
#define HEMOGLOBIN_H

#include <Arduino.h>
#include <stdint.h>
#include "processing.h"

namespace Hemoglobin
{
    enum class Status : uint8_t
    {
        NOT_READY,
        SIGNAL_INVALID,
        CALIBRATION_MISSING,
        INPUT_OUT_OF_RANGE,
        ESTIMATE_AVAILABLE
    };

    struct Result
    {
        Status status = Status::NOT_READY;
        float valueGdL = 0.0f;
        float inputRatio = 0.0f;
        bool available = false;
    };

    // Initialize module.
    void begin();

    // Configure a calibrated linear model:
    // Hb = slope * opticalRatio + intercept
    bool setCalibration(
        float slope,
        float intercept,
        float ratioMin,
        float ratioMax,
        float hbMin,
        float hbMax
    );

    void clearCalibration();

    // Estimate using configured calibration.
    Status estimate(
        const Processing::Features &features,
        Result &result
    );

    // Print an experimental estimate using placeholder coefficients.
    // This is not a validated hemoglobin measurement.
    void printExperimentalEstimate(
        const Processing::Features &features
    );

    const char *statusToString(Status status);
}

#endif
