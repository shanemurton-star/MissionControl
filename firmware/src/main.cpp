#include <Arduino.h>

#include "config/Version.h"
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
#include "services/NetworkUpdateState.h"

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

namespace
{
    TaskHandle_t networkUpdateTaskHandle = nullptr;
    StaticTask_t networkUpdateTaskBuffer;
    StackType_t networkUpdateTaskStack[6144];

    void networkUpdateTask(void*)
    {
        uint8_t serviceSlot = 0;

        for (;;)
        {
            if (wifiService.isNetworkReady() && clockService.isSynchronized())
            {
                NetworkUpdateState::setBusy(true);
                switch (serviceSlot)
                {
                    case 0: weatherService.update(); break;
                    case 1: aircraftService.update(); break;
                    case 2: satelliteService.update(); break;
                    case 3: solarService.update(); break;
                    case 4: liveSpotsService.update(); break;
                    case 5: potaService.update(); break;
                }
                NetworkUpdateState::setBusy(false);
                serviceSlot = (serviceSlot + 1) % 6;
            }

            vTaskDelay(pdMS_TO_TICKS(50));
        }
    }
}

void setup()
{
    Serial.begin(115200);
    delay(500);

    Serial.println();
    Serial.println("==============================");
    Serial.println("Mission Control starting...");
    Serial.println("==============================");
    Serial.print("Firmware build: ");
    Serial.print(__DATE__);
    Serial.print(" ");
    Serial.println(__TIME__);
    Serial.print("Firmware version: ");
    Serial.println(Version::FIRMWARE);
    Serial.println("Network revision: UI-23 / reduced satellite memory");

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

    // The isolated network diagnostic proved that DHCP, numeric NTP, DNS, and
    // HTTPS all work before the display stack is initialized. Give SNTP that
    // same quiet startup window here, before LVGL and the panel screens consume
    // memory and processing time. Both waits are bounded so the UI will still
    // start when the network is unavailable.
    const uint32_t wifiStartupDeadline = millis() + 20000UL;
    while (!wifiService.isNetworkReady() &&
           static_cast<int32_t>(millis() - wifiStartupDeadline) < 0)
    {
        wifiService.update();
        delay(100);
    }

    if (wifiService.isNetworkReady())
    {
        const uint32_t timeStartupDeadline = millis() + 15000UL;
        while (!clockService.isSynchronized() &&
               static_cast<int32_t>(millis() - timeStartupDeadline) < 0)
        {
            clockService.update();
            delay(100);
        }

        if (!clockService.isSynchronized())
        {
            Serial.println("Initial time synchronization timed out; continuing startup.");
        }
    }
    else
    {
        Serial.println("Initial WiFi connection timed out; continuing startup.");
    }

    weatherService.begin(
        settingsService.get());

    aircraftService.begin(
        settingsService.get());

    satelliteService.begin(
        settingsService.get());
    solarService.begin();
    liveSpotsService.begin(settingsService.get());
    potaService.begin(settingsService.get());

    if (wifiService.isNetworkReady() && clockService.isSynchronized())
    {
        Serial.println("Loading initial panel data before display startup...");

        // DNS and HTTPS were verified to work in the isolated pre-display
        // environment. Complete each service's initial state-machine work in
        // that same window, then let the normal loop handle timed refreshes.
        for (uint8_t step = 0; step < 7; ++step)
        {
            weatherService.update();
            delay(10);
        }

        aircraftService.update();

        // DNS on this display becomes unreliable after the RGB/LVGL hardware
        // stack starts. CelesTrak now has bounded connection/handshake limits,
        // so load its five initial TLEs in the proven pre-display DNS window.
        for (uint8_t satellite = 0;
             satellite < SatelliteService::SATELLITE_COUNT;
             ++satellite)
        {
            satelliteService.update();
            delay(10);
        }

        if (satelliteService.isValid())
        {
            for (uint8_t satellite = 0;
                 satellite < SatelliteService::SATELLITE_COUNT;
                 ++satellite)
            {
                satelliteService.requestRadioData(satellite);
                satelliteService.update();
                delay(10);
            }
        }

        solarService.update();
        liveSpotsService.update();
        potaService.update();
        Serial.println("Initial panel data loading complete.");
    }

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

    networkUpdateTaskHandle = xTaskCreateStaticPinnedToCore(
        networkUpdateTask,
        "panel-network",
        sizeof(networkUpdateTaskStack),
        nullptr,
        1,
        networkUpdateTaskStack,
        &networkUpdateTaskBuffer,
        0);

    Serial.println(networkUpdateTaskHandle != nullptr
        ? "Background panel network worker started."
        : "ERROR: Unable to start background panel network worker.");
    Serial.print("Free heap after UI and worker: ");
    Serial.println(ESP.getFreeHeap());
}

void loop()
{
    // LVGL must run before any synchronous network operation so touch and
    // drawing never wait behind a whole batch of API requests.
    displayService.update();
    wifiService.update();
    clockService.update();

    delay(5);
}
