
#include "processing.h"
#include "../../include/config.h"

#include <math.h>
#include <stdint.h>
#include <limits.h>

namespace
{
    // ============================================================
    // CONFIGURATION
    // ============================================================

    constexpr uint16_t WINDOW_SIZE_LOCAL = 500;
    constexpr uint8_t CONTACT_CONFIRM_SAMPLES = 3;

    // Measured sensor rate. Update if your measured rate changes.
    constexpr float FILTER_SAMPLE_RATE_HZ = 77.7f;

    // Initial filter cutoff frequencies.
    constexpr float DC_CUTOFF_HZ = 0.5f;
    constexpr float AC_CUTOFF_HZ = 5.0f;

    constexpr float TWO_PI_F = 6.28318530718f;

    // First-order exponential low-pass filter coefficients.
    const float dcAlpha =
        1.0f - expf(
            -TWO_PI_F * DC_CUTOFF_HZ / FILTER_SAMPLE_RATE_HZ
        );

    const float acAlpha =
        1.0f - expf(
            -TWO_PI_F * AC_CUTOFF_HZ / FILTER_SAMPLE_RATE_HZ
        );

    // ============================================================
    // RAW SAMPLE WINDOW
    // ============================================================

    uint32_t redBuffer[WINDOW_SIZE_LOCAL] = {};
    uint32_t irBuffer[WINDOW_SIZE_LOCAL] = {};
    bool fingerBuffer[WINDOW_SIZE_LOCAL] = {};

    uint16_t writeIndex = 0;
    uint16_t sampleCount = 0;
    bool windowReady = false;

    // ============================================================
    // FINGER DETECTION
    // ============================================================

    bool fingerState = false;

    uint8_t contactOnCount = 0;
    uint8_t contactOffCount = 0;

    // ============================================================
    // FILTER STATE
    // ============================================================

    struct FilterState
    {
        float redDC = 0.0f;
        float irDC = 0.0f;

        float redAC = 0.0f;
        float irAC = 0.0f;

        bool initialized = false;
    };

    FilterState filterState;

    Processing::FilteredSample latestFilteredSample;

    // ============================================================
    // WINDOW MANAGEMENT
    // ============================================================

    void clearWindow()
    {
        writeIndex = 0;
        sampleCount = 0;
        windowReady = false;

        // The old array contents are ignored until overwritten by
        // a complete fresh window.
    }

    // ============================================================
    // FILTER RESET
    // ============================================================

    void resetFilters()
    {
        filterState = FilterState{};
        latestFilteredSample = Processing::FilteredSample{};
    }

    // ============================================================
    // DC AND AC FILTERING
    // ============================================================

    void updateFilters(uint32_t red, uint32_t ir)
    {
        latestFilteredSample.rawRed = red;
        latestFilteredSample.rawIR = ir;

        const float redInput = static_cast<float>(red);
        const float irInput = static_cast<float>(ir);

        // Initialize DC from the first sample to avoid a startup
        // transient caused by an arbitrary initial baseline.
        if (!filterState.initialized)
        {
            filterState.redDC = redInput;
            filterState.irDC = irInput;

            filterState.redAC = 0.0f;
            filterState.irAC = 0.0f;

            filterState.initialized = true;
        }
        else
        {
            // ----------------------------------------------------
            // DC PATH: LOW-PASS FILTER
            // ----------------------------------------------------

            filterState.redDC +=
                dcAlpha * (redInput - filterState.redDC);

            filterState.irDC +=
                dcAlpha * (irInput - filterState.irDC);

            // ----------------------------------------------------
            // AC PATH: REMOVE ESTIMATED DC BASELINE
            // ----------------------------------------------------

            const float redACRaw =
                redInput - filterState.redDC;

            const float irACRaw =
                irInput - filterState.irDC;

            // ----------------------------------------------------
            // AC PATH: LOW-PASS FILTER TO REDUCE HIGH-FREQUENCY NOISE
            // ----------------------------------------------------

            filterState.redAC +=
                acAlpha * (redACRaw - filterState.redAC);

            filterState.irAC +=
                acAlpha * (irACRaw - filterState.irAC);
        }

        latestFilteredSample.redDC = filterState.redDC;
        latestFilteredSample.irDC = filterState.irDC;

        latestFilteredSample.redAC = filterState.redAC;
        latestFilteredSample.irAC = filterState.irAC;

        latestFilteredSample.ready = filterState.initialized;
    }

    // ============================================================
    // QUALITY HELPERS
    // ============================================================

    uint8_t clampQuality(int score)
    {
        if (score < 0)
            return 0;

        if (score > 100)
            return 100;

        return static_cast<uint8_t>(score);
    }
}

// ================================================================
// PROCESSING NAMESPACE
// ================================================================

namespace Processing
{
    // ------------------------------------------------------------
    // INITIALIZATION
    // ------------------------------------------------------------

    void begin()
    {
        reset();
    }

    // ------------------------------------------------------------
    // RESET ALL PROCESSING STATE
    // ------------------------------------------------------------

    void reset()
    {
        for (uint16_t i = 0; i < WINDOW_SIZE_LOCAL; i++)
        {
            redBuffer[i] = 0;
            irBuffer[i] = 0;
            fingerBuffer[i] = false;
        }

        clearWindow();

        fingerState = false;

        contactOnCount = 0;
        contactOffCount = 0;

        resetFilters();
    }

    // ------------------------------------------------------------
    // ADD RAW SAMPLE
    // ------------------------------------------------------------

    void addSample(uint32_t red, uint32_t ir)
    {
        // ========================================================
        // FINGER PLACEMENT DETECTION
        // ========================================================

        if (!fingerState)
        {
            contactOffCount = 0;

            if (ir >= PROCESSING_FINGER_ON_THRESHOLD)
            {
                if (contactOnCount < CONTACT_CONFIRM_SAMPLES)
                    contactOnCount++;
            }
            else
            {
                contactOnCount = 0;
            }

            // Wait for three consecutive samples above the ON
            // threshold before accepting finger contact.
            if (contactOnCount < CONTACT_CONFIRM_SAMPLES)
                return;

            fingerState = true;

            contactOnCount = 0;
            contactOffCount = 0;

            // Begin a new measurement and filter history.
            clearWindow();
            resetFilters();
        }
        else
        {
            // ====================================================
            // FINGER REMOVAL DETECTION
            // ====================================================

            if (ir <= PROCESSING_FINGER_OFF_THRESHOLD)
            {
                if (contactOffCount < CONTACT_CONFIRM_SAMPLES)
                    contactOffCount++;

                contactOnCount = 0;

                // Ignore possible removal samples.
                if (contactOffCount < CONTACT_CONFIRM_SAMPLES)
                    return;

                // Finger removal confirmed.
                fingerState = false;

                contactOnCount = 0;
                contactOffCount = 0;

                // Discard old measurements and filter state.
                clearWindow();
                resetFilters();

                return;
            }

            // Signal recovered above the OFF threshold.
            contactOffCount = 0;
            contactOnCount = 0;
        }

        // ========================================================
        // FILTERING
        // ========================================================

        // Only confirmed finger-contact samples reach the filters.
        updateFilters(red, ir);

        // ========================================================
        // STORE RAW SAMPLES
        // ========================================================

        redBuffer[writeIndex] = red;
        irBuffer[writeIndex] = ir;
        fingerBuffer[writeIndex] = true;

        writeIndex++;

        if (writeIndex >= WINDOW_SIZE_LOCAL)
            writeIndex = 0;

        if (sampleCount < WINDOW_SIZE_LOCAL)
            sampleCount++;

        if (sampleCount >= WINDOW_SIZE_LOCAL)
            windowReady = true;
        static uint32_t lastProgressPrint = 0;
    }

    // ------------------------------------------------------------
    // FILTERED SAMPLE ACCESS
    // ------------------------------------------------------------

    bool getLatestFilteredSample(FilteredSample &sample)
    {
        if (!latestFilteredSample.ready)
        {
            sample = FilteredSample{};
            return false;
        }

        sample = latestFilteredSample;
        return true;
    }

    // ------------------------------------------------------------
    // WINDOW STATUS
    // ------------------------------------------------------------

    bool isWindowReady()
    {
        return windowReady;
    }

    uint16_t getSampleCount()
    {
        return sampleCount;
    }

    // ------------------------------------------------------------
    // FEATURE CALCULATION
    // ------------------------------------------------------------

    bool calculateFeatures(Features &features)
    {
        // Never calculate features using an incomplete window.
        if (!windowReady || sampleCount < WINDOW_SIZE_LOCAL)
        {
            features = Features{};
            features.fingerDetected = fingerState;
            features.windowReady = false;
            features.signalValid = false;

            return false;
        }

        features = Features{};

        features.fingerDetected = fingerState;
        features.windowReady = true;

        if (!fingerState)
        {
            features.signalValid = false;
            return false;
        }

        // ========================================================
        // 1. RAW DC MEAN, PEAK-TO-PEAK AND CLIPPING
        // ========================================================

        double redSum = 0.0;
        double irSum = 0.0;

        uint32_t redMin = UINT32_MAX;
        uint32_t redMax = 0;

        uint32_t irMin = UINT32_MAX;
        uint32_t irMax = 0;

        uint32_t clippedSamples = 0;
        uint32_t contactSamples = 0;

        for (uint16_t i = 0; i < WINDOW_SIZE_LOCAL; i++)
        {
            const uint32_t red = redBuffer[i];
            const uint32_t ir = irBuffer[i];

            redSum += static_cast<double>(red);
            irSum += static_cast<double>(ir);

            if (red < redMin)
                redMin = red;

            if (red > redMax)
                redMax = red;

            if (ir < irMin)
                irMin = ir;

            if (ir > irMax)
                irMax = ir;

            if (fingerBuffer[i])
                contactSamples++;

            if (red >= PROCESSING_ADC_CLIP_THRESHOLD ||
                ir >= PROCESSING_ADC_CLIP_THRESHOLD)
            {
                clippedSamples++;
            }
        }

        const double sampleTotal =
            static_cast<double>(WINDOW_SIZE_LOCAL);

        const double redMean = redSum / sampleTotal;
        const double irMean = irSum / sampleTotal;

        features.redDC = static_cast<float>(redMean);
        features.irDC = static_cast<float>(irMean);

        features.redPeakToPeak = redMax - redMin;
        features.irPeakToPeak = irMax - irMin;

        features.contactPercent =
            static_cast<uint8_t>(
                (contactSamples * 100UL) / WINDOW_SIZE_LOCAL
            );

        features.clippedPercent =
            static_cast<uint8_t>(
                (clippedSamples * 100UL) / WINDOW_SIZE_LOCAL
            );

        // ========================================================
        // 2. RAW AC RMS
        //
        // Retained for backward compatibility with your existing
        // feature output. These values are calculated from the
        // raw window, not from the new filtered AC path.
        // ========================================================

        double redSquaredSum = 0.0;
        double irSquaredSum = 0.0;

        for (uint16_t i = 0; i < WINDOW_SIZE_LOCAL; i++)
        {
            const double redAC =
                static_cast<double>(redBuffer[i]) - redMean;

            const double irAC =
                static_cast<double>(irBuffer[i]) - irMean;

            redSquaredSum += redAC * redAC;
            irSquaredSum += irAC * irAC;
        }

        const double redRMS =
            sqrt(redSquaredSum / sampleTotal);

        const double irRMS =
            sqrt(irSquaredSum / sampleTotal);

        features.redAC_RMS = static_cast<float>(redRMS);
        features.irAC_RMS = static_cast<float>(irRMS);

        // ========================================================
        // 3. NORMALIZED AC AMPLITUDE
        // ========================================================

        if (redMean > 0.0)
        {
            features.redNormalizedAC =
                static_cast<float>(
                    (redRMS / redMean) * 100.0
                );
        }

        if (irMean > 0.0)
        {
            features.irNormalizedAC =
                static_cast<float>(
                    (irRMS / irMean) * 100.0
                );
        }

        // ========================================================
        // 4. OPTICAL RATIO
        //
        // Existing behavior is intentionally preserved here.
        // This still uses raw-window RMS, not filtered AC RMS.
        // ========================================================

        if (redMean > 0.0 &&
            irMean > 0.0 &&
            irRMS > 0.0)
        {
            features.opticalRatio = static_cast<float>(
                (redRMS / redMean) /
                (irRMS / irMean)
            );
        }

        // ========================================================
        // 5. HEURISTIC SIGNAL QUALITY
        // ========================================================

        int quality = 100;

        if (features.contactPercent <
            PROCESSING_MIN_CONTACT_PERCENT)
        {
            quality -= 40;
        }

        if (features.redDC < PROCESSING_MIN_DC_LEVEL ||
            features.irDC < PROCESSING_MIN_DC_LEVEL)
        {
            quality -= 30;
        }

        if (features.clippedPercent >
            PROCESSING_MAX_CLIPPED_PERCENT)
        {
            quality -= 30;
        }

        if (features.redNormalizedAC <
                PROCESSING_MIN_PI_PERCENT ||
            features.irNormalizedAC <
                PROCESSING_MIN_PI_PERCENT)
        {
            quality -= 20;
        }

        if (features.redNormalizedAC >
                PROCESSING_MAX_PI_PERCENT ||
            features.irNormalizedAC >
                PROCESSING_MAX_PI_PERCENT)
        {
            quality -= 20;
        }

        if (features.redAC_RMS <= 0.0f ||
            features.irAC_RMS <= 0.0f)
        {
            quality -= 20;
        }

        features.signalQuality = clampQuality(quality);

        // ========================================================
        // 6. SIGNAL VALIDITY
        // ========================================================

        const bool dcValid =
            features.redDC >= PROCESSING_MIN_DC_LEVEL &&
            features.irDC >= PROCESSING_MIN_DC_LEVEL;

        const bool acValid =
            features.redAC_RMS > 0.0f &&
            features.irAC_RMS > 0.0f;

        const bool perfusionValid =
            features.redNormalizedAC >=
                PROCESSING_MIN_PI_PERCENT &&
            features.redNormalizedAC <=
                PROCESSING_MAX_PI_PERCENT &&
            features.irNormalizedAC >=
                PROCESSING_MIN_PI_PERCENT &&
            features.irNormalizedAC <=
                PROCESSING_MAX_PI_PERCENT;

        const bool clippingValid =
            features.clippedPercent <=
                PROCESSING_MAX_CLIPPED_PERCENT;

        features.signalValid =
            features.fingerDetected &&
            features.windowReady &&
            features.contactPercent >=
                PROCESSING_MIN_CONTACT_PERCENT &&
            features.signalQuality >=
                PROCESSING_MIN_QUALITY_SCORE &&
            dcValid &&
            acValid &&
            perfusionValid &&
            clippingValid;

        return true;
    }
}
