#include <Arduino.h>
#include "hardware/DisplayService.h"

DisplayService displayService;

void setup()
{
    Serial.begin(115200);
    delay(500);

    Serial.println();
    Serial.println("==============================");
    Serial.println("Mission Control starting...");
    Serial.println("==============================");

    if (displayService.begin())
    {
        Serial.println("Display service initialized.");
    }
    else
    {
        Serial.println("ERROR: Display service failed to initialize.");
    }
}

void loop()
{
    displayService.update();
    delay(5);
}