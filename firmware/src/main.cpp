#include <Arduino.h>

#include "services/WiFiService.h"
#include "services/ClockService.h"

#include "screens/ScreenManager.h"
#include "screens/HomeScreen.h"


WiFiService wifiService;
ClockService clockService;

ScreenManager screenManager;
HomeScreen homeScreen;


void setup()
{
    Serial.begin(115200);

    delay(1000);

    Serial.println();
    Serial.println("============================");
    Serial.println("       MISSION CONTROL");
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


    screenManager.begin();

    Serial.println("Mission Control ready");
}


void loop()
{
    if (wifiService.isConnected())
    {
        homeScreen.draw(
            clockService,
            wifiService
        );
    }
    else
    {
        Serial.println("Waiting for network...");
    }


    screenManager.update();


    delay(1000);
}