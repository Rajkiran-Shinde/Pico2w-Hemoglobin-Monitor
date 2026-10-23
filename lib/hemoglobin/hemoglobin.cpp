
#include "hemoglobin.h"

#include <math.h>

namespace
{
    struct Calibration
    {
        float slope = 0.0f;
        float intercept = 0.0f;
        float ratioMin = 0.0f;
        float ratioMax = 0.0f;
        float hbMin = 0.0f;
        float hbMax = 0.0f;
        bool configured = false;
    };

    Calibration calibration;

    bool isFiniteNumber(float value)
    {
        return isfinite(value);
    }

    bool validCalibration(
        float slope,
        float intercept,
        float ratioMin,
        float ratioMax,
        float hbMin,
        float hbMax
    )
    {
        return isFiniteNumber(slope) &&
               isFiniteNumber(intercept) &&
               isFiniteNumber(ratioMin) &&
               isFiniteNumber(ratioMax) &&
               isFiniteNumber(hbMin) &&
               isFiniteNumber(hbMax) &&
               ratioMin >= 0.0f &&
               ratioMax > ratioMin &&
               hbMin >= 0.0f &&
               hbMax > hbMin;
    }
}

namespace Hemoglobin
{
    // --------------------------------------------------------
    // INITIALIZATION
    // --------------------------------------------------------

    void begin()
    {
        clearCalibration();
    }

    // --------------------------------------------------------
    // CALIBRATION
    // --------------------------------------------------------

    bool setCalibration(
        float slope,
        float intercept,
        float ratioMin,
        float ratioMax,
        float hbMin,
        float hbMax
    )
    {
        if (!validCalibration(
                slope,
                intercept,
                ratioMin,
                ratioMax,
                hbMin,
                hbMax))
        {
            clearCalibration();
            return false;
        }

        calibration.slope = slope;
        calibration.intercept = intercept;
        calibration.ratioMin = ratioMin;
        calibration.ratioMax = ratioMax;
        calibration.hbMin = hbMin;
        calibration.hbMax = hbMax;
        calibration.configured = true;

        return true;
    }

    void clearCalibration()
    {
        calibration = Calibration{};
    }

    // --------------------------------------------------------
    // CALIBRATED ESTIMATION
    // --------------------------------------------------------

    Status estimate(
        const Processing::Features &features,
        Result &result
    )
    {
        result = Result{};

        if (!features.fingerDetected || !features.windowReady)
        {
            result.status = Status::NOT_READY;
            return result.status;
        }

        if (!features.signalValid)
        {
            result.status = Status::SIGNAL_INVALID;
            return result.status;
        }

        const float ratio = features.opticalRatio;

        if (!isFiniteNumber(ratio) || ratio <= 0.0f)
        {
            result.inputRatio = ratio;
            result.status = Status::INPUT_OUT_OF_RANGE;
            return result.status;
        }

        // If a real calibration is configured, use it and enforce its limits.
        if (calibration.configured)
        {
            if (ratio < calibration.ratioMin || ratio > calibration.ratioMax)
            {
                result.inputRatio = ratio;
                result.status = Status::INPUT_OUT_OF_RANGE;
                return result.status;
            }

            const float hb = calibration.slope * ratio + calibration.intercept;
            if (!isFiniteNumber(hb) || hb < calibration.hbMin || hb > calibration.hbMax)
            {
                result.inputRatio = ratio;
                result.status = Status::INPUT_OUT_OF_RANGE;
                return result.status;
            }

            result.status = Status::ESTIMATE_AVAILABLE;
            result.valueGdL = hb;
            result.inputRatio = ratio;
            result.available = true;
            return result.status;
        }

        /*
         * UNCALIBRATED DEMONSTRATION FALLBACK.
         * This is NOT trained on the published dataset and is NOT medically validated.
         * It only allows firmware/UI testing without a calibration prompt.
         * Do not use this value for medical decisions.
         */
        constexpr float EXPERIMENTAL_SLOPE = 1.0f;
        constexpr float EXPERIMENTAL_INTERCEPT = 12.0f;
        const float hb = EXPERIMENTAL_SLOPE * ratio + EXPERIMENTAL_INTERCEPT;
        if (!isFiniteNumber(hb) || hb <= 0.0f)
        {
            result.inputRatio = ratio;
            result.status = Status::INPUT_OUT_OF_RANGE;
            return result.status;
        }

        result.status = Status::EXPERIMENTAL_ESTIMATE;
        result.valueGdL = hb;
        result.inputRatio = ratio;
        result.available = true;
        return result.status;
    }

    // --------------------------------------------------------
    // EXPERIMENTAL ESTIMATE FOR SERIAL MONITOR
    // --------------------------------------------------------

    void printExperimentalEstimate(
        const Processing::Features &features
    )
    {
        if (!features.fingerDetected || !features.windowReady)
        {
            Serial.println(
                "Experimental Hb: Waiting for a complete window"
            );
            return;
        }

        if (!features.signalValid)
        {
            Serial.println(
                "Experimental Hb: Invalid signal"
            );
            return;
        }

        const float ratio = features.opticalRatio;

        if (!isFiniteNumber(ratio) || ratio <= 0.0f)
        {
            Serial.println(
                "Experimental Hb: Invalid optical ratio"
            );
            return;
        }

        /*
         * DEMONSTRATION ONLY.
         *
         * These coefficients are placeholders, not coefficients
         * derived from the downloaded dataset or reference tests.
         *
         * Replace them only after validating a calibration model.
         */
        constexpr float DEMO_SLOPE = 1.0f;
        constexpr float DEMO_INTERCEPT = 12.0f;

        const float estimatedHb =
            DEMO_SLOPE * ratio + DEMO_INTERCEPT;

        Serial.print("Optical ratio: ");
        Serial.println(ratio, 4);

        Serial.print("Experimental Hb output: ");
        Serial.print(estimatedHb, 2);
        Serial.println(" g/dL ");
    }

    // --------------------------------------------------------
    // STATUS TEXT
    // --------------------------------------------------------

    const char *statusToString(Status status)
    {
        switch (status)
        {
            case Status::NOT_READY:
                return "WAITING_FOR_FRESH_WINDOW";

            case Status::SIGNAL_INVALID:
                return "SIGNAL_INVALID";

            case Status::CALIBRATION_MISSING:
                return "CALIBRATION_REQUIRED";

            case Status::INPUT_OUT_OF_RANGE:
                return "INPUT_OUT_OF_RANGE";

            case Status::ESTIMATE_AVAILABLE:
                return "ESTIMATE_AVAILABLE";

            case Status::EXPERIMENTAL_ESTIMATE:
                return "EXP - NOT CALIBRATED";

            default:
                return "UNKNOWN_STATUS";
        }
    }
}
