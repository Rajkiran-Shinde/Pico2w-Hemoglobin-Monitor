#ifndef HEMOGLOBIN_H
#define HEMOGLOBIN_H

#include <Arduino.h>
#include "processing.h"

namespace Hemoglobin {

enum class Status : uint8_t {
    NOT_READY,
    SIGNAL_INVALID,
    CALIBRATION_MISSING,
    INPUT_OUT_OF_RANGE,
    ESTIMATE_AVAILABLE,
    EXPERIMENTAL_ESTIMATE
};

struct Result {
    Status status = Status::NOT_READY;
    float valueGdL = 0.0f;
    float inputRatio = 0.0f;
    bool available = false;
};

void begin();

bool setCalibration(
    float slope,
    float intercept,
    float ratioMin,
    float ratioMax,
    float hbMin,
    float hbMax
);

void clearCalibration();

Status estimate(const Processing::Features &features, Result &result);

void printExperimentalEstimate(const Processing::Features &features);

const char *statusToString(Status status);

} // namespace Hemoglobin

#endif // HEMOGLOBIN_H
