#include <Arduino.h>
#include "hardware/DisplayService.h"

// Hardware services
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

    // Brief delay prevents the loop from monopolizing the processor.
    delay(5);
}