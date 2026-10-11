
#include <Arduino.h>
#include <Wire.h>
#include "MAX30105.h"

// MAX30102 sensor object
MAX30105 particleSensor;

void setup()
{
    // Start Serial Monitor
    Serial.begin(115200);
    delay(2000);

    Serial.println();
    Serial.println("================================");
    Serial.println("   MAX30102 SENSOR TEST");
    Serial.println("================================");

    // Initialize I2C using the default Pico pins:
    // SDA = GP4 (physical pin 6)
    // SCL = GP5 (physical pin 7)
    Wire.begin();

    Serial.println("Initializing MAX30102...");

    // Check sensor communication
    if (!particleSensor.begin(Wire, I2C_SPEED_STANDARD))
    {
        Serial.println("ERROR: MAX30102 not detected!");
        Serial.println("Check wiring and power supply.");

        while (true)
        {
            delay(1000);
            Serial.println("Waiting for sensor...");
        }
    }

    Serial.println("MAX30102 detected successfully!");

    // Configure sensor
    byte ledBrightness = 60;  // LED brightness: 0-255
    byte sampleAverage = 4;   // Average 4 samples
    byte ledMode = 2;          // Red + IR LEDs
    int sampleRate = 100;      // 100 samples per second
    int pulseWidth = 411;      // ADC resolution setting
    int adcRange = 4096;       // ADC range

    particleSensor.setup(
        ledBrightness,
        sampleAverage,
        ledMode,
        sampleRate,
        pulseWidth,
        adcRange
    );

    Serial.println("Sensor configured!");
    Serial.println("Place your finger on the sensor.");
    Serial.println();
}

void loop()
{
    // Read raw infrared and red light values
    long irValue = particleSensor.getIR();
    long redValue = particleSensor.getRed();

    // Print readings
    Serial.print("IR Value: ");
    Serial.print(irValue);

    Serial.print("  |  RED Value: ");
    Serial.print(redValue);

    // Basic finger detection indication
    if (irValue > 50000)
    {
        Serial.println("  |  Finger detected");
    }
    else
    {
        Serial.println("  |  No finger / weak signal");
    }

    delay(250);
}
