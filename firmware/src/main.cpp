#include <Arduino.h>
#include "services/ClockService.h"

ClockService clockService;


void setup()
{
    Serial.begin(115200);

    delay(1000);

    Serial.println();
    Serial.println("============================");
    Serial.println("   MISSION CONTROL");
    Serial.println("============================");

    clockService.begin();
}


void loop()
{
    Serial.print("LOCAL: ");
    Serial.println(clockService.getLocalTime());

    Serial.print("UTC:   ");
    Serial.println(clockService.getUTCTime());

    Serial.println();

    delay(1000);
}