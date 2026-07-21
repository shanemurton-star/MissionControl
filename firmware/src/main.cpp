#include <Arduino.h>

#include "services/WiFiService.h"
#include "services/ClockService.h"
#include "services/StatusService.h"

#include "screens/ScreenManager.h"
#include "screens/HomeScreen.h"


WiFiService wifiService;
ClockService clockService;

StatusService statusService;

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


    statusService.begin();

    wifiService.begin();


    if (wifiService.isConnected())
    {
        statusService.setStatus(
            "WIFI",
            "CONNECTED"
        );

        clockService.begin();

        statusService.setStatus(
            "TIME",
            "SYNCED"
        );

        Serial.println("Clock service started");
    }
    else
    {
        statusService.setStatus(
            "WIFI",
            "OFFLINE"
        );

        statusService.setStatus(
            "TIME",
            "WAITING"
        );

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


    statusService.printStatus();


    screenManager.update();


    delay(1000);
}