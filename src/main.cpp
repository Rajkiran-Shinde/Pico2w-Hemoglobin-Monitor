
#include <Arduino.h>
#include <Wire.h>
#include "config.h"
#include "max30102.h"

void setup()
{
    Serial.begin(SERIAL_BAUD_RATE);
    delay(2000);

    // Use the default Pico I2C pins: GP4 SDA, GP5 SCL.
    Wire.begin();

    Serial.println("MAX30102 Driver Test");

    if (!MAX30102::begin(Wire))
    {
        Serial.println("Sensor initialization failed!");
        while (true)
        {
            delay(1000);
        }
    }

    Serial.println("MAX30102 initialized successfully.");
}

void loop()
{
    MAX30102::SensorData data;

    if (MAX30102::readSensor(data))
    {
        Serial.print("RED: ");
        Serial.print(data.red);

        Serial.print(" | IR: ");
        Serial.print(data.ir);

        Serial.print(" | Finger: ");
        Serial.println(data.fingerDetected ? "YES" : "NO");
    }

    delay(SENSOR_UPDATE_INTERVAL_MS);
}
