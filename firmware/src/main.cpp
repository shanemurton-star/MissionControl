#include <Arduino.h>

#include "services/WiFiService.h"
#include "services/ClockService.h"


WiFiService wifiService;
ClockService clockService;


void setup()
{
    Serial.begin(115200);

    delay(1000);

    Serial.println();
    Serial.println("============================");
    Serial.println("   MISSION CONTROL");
    Serial.println("============================");

    wifiService.begin();

    if (wifiService.isConnected())
    {
        clockService.begin();
        Serial.println("Clock service started");
    }
    else
    {
        Serial.println("Clock service waiting for WiFi");
    }
}


void loop()
{
    Serial.println();

    if (wifiService.isConnected())
    {
        Serial.print("LOCAL: ");
        Serial.println(clockService.getLocalTime());

        Serial.print("UTC:   ");
        Serial.println(clockService.getUTCTime());
    }
    else
    {
        Serial.println("Waiting for network...");
    }

    delay(1000);
}