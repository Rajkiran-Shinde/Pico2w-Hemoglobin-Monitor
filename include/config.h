
#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ============================================================
// PROJECT CONFIGURATION
// Project: Hemoglobin Monitoring and Data Logging Device
// MCU: Raspberry Pi Pico / Pico W family
// Framework: PlatformIO + Arduino
// ============================================================


// ============================================================
// 1. SERIAL CONFIGURATION
// ============================================================

constexpr uint32_t SERIAL_BAUD_RATE = 115200;


// ============================================================
// 2. I2C CONFIGURATION
// Shared bus: MAX30102 + OLED + RTC + INA219
// ============================================================

constexpr uint8_t PIN_I2C_SDA = 4;  // GP4, physical pin 6
constexpr uint8_t PIN_I2C_SCL = 5;  // GP5, physical pin 7

constexpr uint32_t I2C_CLOCK_HZ = 100000; // 100 kHz

// Common default I2C addresses.
// Verify these against the actual modules.

constexpr uint8_t MAX30102_I2C_ADDRESS = 0x57;
constexpr uint8_t OLED_I2C_ADDRESS     = 0x3C;
constexpr uint8_t RTC_I2C_ADDRESS      = 0x68; // DS3231
constexpr uint8_t INA219_I2C_ADDRESS   = 0x40;


// ============================================================
// 3. MAX30102 CONFIGURATION
// ============================================================

constexpr uint8_t PIN_MAX30102_INT = 17; // GP17, physical pin 22

constexpr uint8_t MAX30102_LED_BRIGHTNESS = 60;
constexpr uint8_t MAX30102_SAMPLE_AVERAGE = 1;
constexpr uint8_t MAX30102_LED_MODE       = 2;   // Red + IR
constexpr uint16_t MAX30102_SAMPLE_RATE   = 200;
constexpr uint16_t MAX30102_PULSE_WIDTH   = 411;
constexpr uint16_t MAX30102_ADC_RANGE     = 4096;


 // ============================================================
 // 3A. WINDOW-LEVEL SIGNAL VALIDATION
 // Initial engineering thresholds; tune using recorded data.
// ============================================================

// Finger-detection hysteresis.
// Use a lower threshold to release contact than to detect it.
constexpr uint32_t PROCESSING_FINGER_ON_THRESHOLD  = 50000;
constexpr uint32_t PROCESSING_FINGER_OFF_THRESHOLD = 40000;

// MAX30102 nominal 18-bit ADC output.
constexpr uint32_t PROCESSING_ADC_MAX = 262143;

// Reject readings close to ADC saturation.
constexpr uint32_t PROCESSING_ADC_CLIP_THRESHOLD = 260000;

// Reject near-zero optical levels.
constexpr uint32_t PROCESSING_MIN_DC_LEVEL = 1000;

// Minimum and maximum normalized pulsatile RMS (% of DC).
constexpr float PROCESSING_MIN_PI_PERCENT = 0.02f;
constexpr float PROCESSING_MAX_PI_PERCENT = 5.0f;

// Minimum proportion of samples with detected finger contact.
constexpr uint8_t PROCESSING_MIN_CONTACT_PERCENT = 90;

// Minimum overall quality score required for a valid window.
constexpr uint8_t PROCESSING_MIN_QUALITY_SCORE = 60;

// Maximum permitted fraction of clipped samples in a window (%).
constexpr uint8_t PROCESSING_MAX_CLIPPED_PERCENT = 1;



// ============================================================
// 4. BUTTON CONFIGURATION
// Buttons will use INPUT_PULLUP.
// Pressed = LOW, Released = HIGH.
// ============================================================

constexpr uint8_t PIN_BUTTON_MODE   = 10; // GP10, physical pin 14
constexpr uint8_t PIN_BUTTON_SELECT = 11; // GP11, physical pin 15

constexpr uint32_t BUTTON_DEBOUNCE_MS = 40;


// ============================================================
// 5. BUZZER CONFIGURATION
// ============================================================

constexpr uint8_t PIN_BUZZER = 12; // GP12, physical pin 16


// ============================================================
// 6. LED CONFIGURATION
// ============================================================

constexpr uint8_t PIN_LED_RED    = 14; // GP14, 
constexpr uint8_t PIN_LED_GREEN  = 13; // GP13, 
constexpr uint8_t PIN_LED_BLUE   = 16; // GP16, 
constexpr uint8_t PIN_LED_YELLOW = 15; // GP15, 


// ============================================================
// 7. APPLICATION TIMING
// ============================================================

constexpr uint32_t SENSOR_UPDATE_INTERVAL_MS = 250;
constexpr uint32_t DISPLAY_UPDATE_INTERVAL_MS = 250;


// ============================================================
// 8. LOCAL STORAGE CONFIGURATION
// ============================================================

// Maximum number of readings held in RAM at once.
// Persistent storage implementation will be added later.

constexpr uint16_t MAX_RAM_LOG_ENTRIES = 100;


// ============================================================
// END OF CONFIGURATION
// ============================================================

#endif // CONFIG_H
