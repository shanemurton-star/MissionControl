#include <Arduino.h>

#include "hardware/DisplayService.h"
#include "services/SettingsService.h"
#include "services/WeatherService.h"
#include "services/WiFiService.h"
#include "services/ClockService.h"

SettingsService settingsService;
DisplayService displayService;
WiFiService wifiService;
ClockService clockService;
WeatherService weatherService;

void setup()
{
    Serial.begin(115200);
    delay(500);

    Serial.println();
    Serial.println("==============================");
    Serial.println("Mission Control starting...");
    Serial.println("==============================");

    /*
     * Load all persistent settings before starting any
     * service that depends on them.
     */
    if (!settingsService.begin())
    {
        Serial.println(
            "ERROR: Settings service failed to initialize.");
    }

    /*
     * Start Wi-Fi using the loaded settings.
     */
    wifiService.begin(
        settingsService.get());

    /*
     * Initialize the weather service.
     * It will wait for Wi-Fi before contacting the NWS.
     *
     * Weather will be connected to SettingsService in the
     * next step.
     */
    clockService.begin();
     weatherService.begin(
    settingsService.get());

    if (displayService.begin(
        clockService,
        weatherService))
    {
        Serial.println(
            "Display service initialized.");
    }
    else
    {
        Serial.println(
            "ERROR: Display service failed to initialize.");
    }
}

void loop()
{
    wifiService.update();
    weatherService.update();
    displayService.update();

    delay(5);
}