#pragma once

#include <Arduino.h>

#include "../screens/ScreenManager.h"
#include "../services/ClockService.h"
#include "../services/AircraftService.h"
#include "../services/SatelliteService.h"
#include "../services/SolarService.h"
#include "../services/LiveSpotsService.h"
#include "../services/WeatherService.h"
#include "../services/SettingsService.h"
#include "../services/WiFiService.h"
#include "../services/PotaService.h"

class DisplayService
{
public:
    bool begin(
        ClockService& clockService,
        WeatherService& weatherService,
        AircraftService& aircraftService,
        SatelliteService& satelliteService,
        SolarService& solarService,
        LiveSpotsService& liveSpotsService,
        PotaService& potaService,
        SettingsService& settingsService,
        WiFiService& wifiService);

    void update();

private:
    void turnOffScreen();

    uint32_t lastTickMillis = 0;

    ScreenManager screenManager;
};
