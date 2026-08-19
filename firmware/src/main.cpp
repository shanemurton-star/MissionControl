#include <Arduino.h>

#include "hardware/DisplayService.h"
#include "services/SettingsService.h"
#include "services/RadarService.h"
#include "services/WeatherService.h"
#include "services/WiFiService.h"
#include "services/ClockService.h"
#include "services/AircraftService.h"
#include "services/SatelliteService.h"
#include "services/SolarService.h"
#include "services/LiveSpotsService.h"
#include "services/PotaService.h"

SettingsService settingsService;
DisplayService displayService;
WiFiService wifiService;
ClockService clockService;
WeatherService weatherService;
RadarService radarService;
AircraftService aircraftService;
SatelliteService satelliteService;
SolarService solarService;
LiveSpotsService liveSpotsService;
PotaService potaService;

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

    radarService.begin(
        settingsService.get());

    aircraftService.begin(
        settingsService.get());

    satelliteService.begin(
        settingsService.get());
    solarService.begin();
    liveSpotsService.begin(settingsService.get());
    potaService.begin(settingsService.get());

    if (displayService.begin(
        clockService,
        weatherService,
        radarService,
        aircraftService,
        satelliteService,
        solarService,
        liveSpotsService,
        potaService,
        settingsService,
        wifiService))
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
    radarService.update();
    aircraftService.update();
    satelliteService.update();
    solarService.update();
    liveSpotsService.update();
    potaService.update();
    displayService.update();

    delay(5);
}
