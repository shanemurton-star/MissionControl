#include <Arduino.h>

void setup()
{
    Serial.begin(115200);

    delay(1000);

    Serial.println();
    Serial.println("================================");
    Serial.println("      MISSION CONTROL");
    Serial.println("      Version 0.1.0");
    Serial.println("================================");
    Serial.println("System initialized");
}

void loop()
{
    Serial.println("Systems nominal");

    delay(5000);
}