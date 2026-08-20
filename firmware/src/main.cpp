#include <Arduino.h>

#include "hardware/DisplayService.h"
#include "services/SettingsService.h"
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
    // LVGL must run before any synchronous network operation so touch and
    // drawing never wait behind a whole batch of API requests.
    displayService.update();
    wifiService.update();
    clockService.update();

    static uint8_t serviceSlot = 0;
    static uint32_t nextServiceMs = 0;
    static uint32_t networkServicesStartMs = 0;

    // Give SNTP exclusive use of the newly connected network before starting
    // HTTPS/DNS traffic. If NTP is unavailable, do not suppress useful data
    // forever; fall back after 20 seconds.
    if (wifiService.isConnected() && networkServicesStartMs == 0)
        networkServicesStartMs = millis() + 20000UL;

    const bool timeReady = clockService.isSynchronized();
    const bool ntpWaitExpired = networkServicesStartMs != 0 &&
        static_cast<int32_t>(millis() - networkServicesStartMs) >= 0;

    if ((timeReady || ntpWaitExpired) &&
        static_cast<int32_t>(millis() - nextServiceMs) >= 0)
    {
        // Run at most one service state per pass. Several services have
        // multi-request startup sequences; round-robin prevents them from
        // monopolizing the UI loop back-to-back.
        switch (serviceSlot)
        {
            case 0: weatherService.update(); break;
            case 1: aircraftService.update(); break;
            case 2: satelliteService.update(); break;
            case 3: solarService.update(); break;
            case 4: liveSpotsService.update(); break;
            case 5: potaService.update(); break;
        }
        serviceSlot = (serviceSlot + 1) % 6;
        nextServiceMs = millis() + 25UL;

        // Catch LVGL up immediately after a request that took noticeable time.
        displayService.update();
    }

    delay(5);
}
