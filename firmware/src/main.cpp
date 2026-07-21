#include <Arduino.h>

#include "services/WiFiService.h"
#include "services/ClockService.h"
#include "services/StatusService.h"
#include "services/WeatherService.h"

#include "screens/ScreenManager.h"
#include "screens/HomeScreen.h"


WiFiService wifiService;
ClockService clockService;

StatusService statusService;
WeatherService weatherService;

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

    weatherService.begin();


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
    }


    screenManager.begin();


    Serial.println("Mission Control ready");
}


void loop()
{
    weatherService.update(
        statusService
    );


    homeScreen.draw(
        clockService,
        statusService
    );


    screenManager.update();


    delay(1000);
}