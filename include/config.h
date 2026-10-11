
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
constexpr uint8_t MAX30102_SAMPLE_AVERAGE = 4;
constexpr uint8_t MAX30102_LED_MODE       = 2;   // Red + IR
constexpr uint16_t MAX30102_SAMPLE_RATE   = 100;
constexpr uint16_t MAX30102_PULSE_WIDTH   = 411;
constexpr uint16_t MAX30102_ADC_RANGE     = 4096;


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

constexpr uint8_t PIN_LED_RED    = 13; // GP13, physical pin 17
constexpr uint8_t PIN_LED_GREEN  = 14; // GP14, physical pin 19
constexpr uint8_t PIN_LED_BLUE   = 15; // GP15, physical pin 20
constexpr uint8_t PIN_LED_YELLOW = 16; // GP16, physical pin 21


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
