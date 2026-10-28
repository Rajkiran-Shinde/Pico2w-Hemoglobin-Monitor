#include <Arduino.h>
#include <Wire.h>
#include <string.h>
#include "../include/config.h"
#include "max30102.h"
#include "processing.h"
#include "hemoglobin.h"
#include "rtc.h"
#include "buttons.h"
#include "display.h"
#include "storage.h"
#include "mode.h"
#include "led.h"
#include "buzz.h"

static bool rtcAvailable = false;
static bool displayAvailable = false;
static bool storageAvailable = false;
static uint32_t lastEstimateMs = 0, lastDisplayMs = 0, lastSequence = 0;
static bool samplingIntegrityValid = true;
static float latestHb = 0.0f;
static bool latestHbAvailable = false;
static char latestStatus[40] = "Waiting for finger";
static char timestamp[24] = "RTC unavailable";

static void setStatus(const char *text) {
  strncpy(latestStatus, text, sizeof(latestStatus) - 1);
  latestStatus[sizeof(latestStatus) - 1] = '\0';
}

void setup() {
  Serial.begin(SERIAL_BAUD_RATE);
  delay(800);
  Serial.println("\nPico 2W Hemoglobin Monitor - UI integration");
// Arduino Mbed RP2040 core
// Default Wire pins: GP4 (SDA), GP5 (SCL)
Wire.begin();
Wire.setClock(I2C_CLOCK_HZ);
  Wire.setClock(I2C_CLOCK_HZ);

  Led::begin();
    Led::setState(Led::State::NORMAL);
    Led::update();
  Buzz::begin();
  Processing::begin();
  Hemoglobin::begin(); // Existing Hb API preserved; no calibration coefficients invented.
  Buttons::begin();
  Mode::begin();

  if (!MAX30102::begin()) {
    setStatus("MAX30102 init failed");
    Mode::setMessage("MAX30102 init failed", 3000);
    Serial.println("ERROR: MAX30102 initialization failed");
    // Keep UI alive so the failure is visible; sensor reads are skipped below.
  } else Serial.println("MAX30102 initialized");

  rtcAvailable = RTC::begin();
  if (rtcAvailable) Serial.println("RTC initialized");
  else Serial.println("WARNING: RTC unavailable");

  displayAvailable = Display::begin();
  if (!displayAvailable) Serial.println("WARNING: OLED initialization failed");

  storageAvailable = Storage::begin();
  if (storageAvailable) Serial.println("LittleFS log storage initialized");
  else Serial.println("WARNING: LittleFS unavailable; logs will not persist");

  if (!storageAvailable) Mode::setMessage("Storage unavailable", 3000);

  // Non-blocking startup melody; Buzz::update() advances it from loop().
  Buzz::playStartupTune();
}

void loop() {
  const Buttons::Event buttonEvent = Buttons::poll();
  if (buttonEvent != Buttons::Event::NONE) Buzz::playButtonBeep();
  Mode::handle(buttonEvent);

  Buzz::update();
  Led::update();

  if (MAX30102::isInitialized()) {
    MAX30102::update();
    static uint32_t lastOverflowCount = 0;
    uint32_t overflows = MAX30102::getQueueOverflowCount();
    if (overflows != lastOverflowCount) {
      lastOverflowCount = overflows;
      Processing::reset(); lastSequence = 0; samplingIntegrityValid = false;
      setStatus("Sampling gap - retry");
    }
    MAX30102::RawSample sample;
    while (MAX30102::readSample(sample)) {
      if (lastSequence != 0 && sample.sequence != lastSequence + 1) {
        Processing::reset(); samplingIntegrityValid = false;
        setStatus("Signal gap - retry");
      }
      lastSequence = sample.sequence;
      Processing::addSample(sample.red, sample.ir);
      if (Processing::getSampleCount() == Processing::WINDOW_SIZE) samplingIntegrityValid = true;
    }

    if (Mode::measurementActive() && !Mode::singleMeasurementComplete() &&
        samplingIntegrityValid && Processing::isWindowReady() && millis() - lastEstimateMs >= 1000) {
      lastEstimateMs = millis();
      Processing::Features features;
      if (Processing::calculateFeatures(features)) {
        if (!features.fingerDetected) {
          setStatus("Place finger on sensor"); latestHbAvailable = false;
        } else if (!features.signalValid) {
          setStatus("Signal invalid - retry"); latestHbAvailable = false;
        } else {
          Hemoglobin::Result result;
          Hemoglobin::Status status = Hemoglobin::estimate(features, result);
          if ((status == Hemoglobin::Status::ESTIMATE_AVAILABLE ||
               status == Hemoglobin::Status::EXPERIMENTAL_ESTIMATE) && result.available) {
            latestHb = result.valueGdL;
            latestHbAvailable = true;
            const bool calibrated = (status == Hemoglobin::Status::ESTIMATE_AVAILABLE);
            setStatus(calibrated ? "Signal OK" : "EXP - NOT CALIBRATED");
            if (rtcAvailable) RTC::getTimestamp(timestamp, sizeof(timestamp));
            else strncpy(timestamp, "RTC unavailable", sizeof(timestamp));
            Mode::onMeasurement(latestHb, timestamp, true,
                                calibrated ? "OK" : "EXPERIMENTAL");
            Serial.print(calibrated ? "Hb calibrated: " : "Hb experimental (NOT calibrated): ");
            Serial.print(latestHb, 2);
            Serial.println(" g/dL");
            Serial.print("Optical ratio: ");
            Serial.println(result.inputRatio, 4);
          } else {
            latestHbAvailable = false;
            setStatus(Hemoglobin::statusToString(status));
            if (status == Hemoglobin::Status::CALIBRATION_MISSING) {
              setStatus("EXP - NOT CALIBRATED");
            } else if (status == Hemoglobin::Status::INPUT_OUT_OF_RANGE) {
              Mode::onMeasurement(0, timestamp, false, "Signal out of range");
            }
          }
        }
      } else { latestHbAvailable = false; setStatus("Signal invalid - retry"); }
    }
  }

  if (rtcAvailable) RTC::getTimestamp(timestamp, sizeof(timestamp));
  if (millis() - lastDisplayMs >= DISPLAY_UPDATE_INTERVAL_MS) {
    lastDisplayMs = millis();
    uint16_t n = Processing::getSampleCount();
    uint8_t pct = (uint8_t)((uint32_t)n * 100U / Processing::WINDOW_SIZE);
    Display::update(Mode::screen(), Mode::measureMode(), Mode::message(), timestamp,
                    rtcAvailable, latestHb, latestHbAvailable, latestStatus, pct,
                    Mode::menuSelection(), Mode::settingsMenuSelection());
  }

  // Indicator priority: sensor initialization failure > recoverable fault >
  // measurement/result activity > normal ready state.
  if (!MAX30102::isInitialized()) {
    Led::setState(Led::State::CRITICAL);
  } else {
    const bool signalFault =
        (strstr(latestStatus, "invalid") != nullptr) ||
        (strstr(latestStatus, "gap") != nullptr) ||
        (strstr(latestStatus, "retry") != nullptr) ||
        (strstr(latestStatus, "failed") != nullptr);
    const bool peripheralWarning = !rtcAvailable || !storageAvailable;

    if (signalFault || peripheralWarning) {
      Led::setState(Led::State::WARNING);
    } else if (Mode::measurementActive() || latestHbAvailable) {
      Led::setState(Led::State::ACTIVITY);
    } else {
      Led::setState(Led::State::NORMAL);
    }
  }
}
