#pragma once

#include <lvgl.h>

#include "DashboardScreen.h"
#include "AircraftScreen.h"
#include "SatelliteScreen.h"
#include "SolarScreen.h"
#include "LiveSpotsScreen.h"
#include "StartupScreen.h"
#include "WeatherScreen.h"
#include "SettingsScreen.h"
#include "PotaScreen.h"

#include "../models/Page.h"
#include "../services/ClockService.h"
#include "../services/AircraftService.h"
#include "../services/SatelliteService.h"
#include "../services/SolarService.h"
#include "../services/LiveSpotsService.h"
#include "../services/WeatherService.h"
#include "../services/SettingsService.h"
#include "../services/WiFiService.h"
#include "../services/PotaService.h"

class ScreenManager
{
public:
    void begin(
        ClockService& clockService,
        WeatherService& weatherService,
        AircraftService& aircraftService,
        SatelliteService& satelliteService,
        SolarService& solarService,
        LiveSpotsService& liveSpotsService,
        PotaService& potaService,
        SettingsService& settingsService,
        WiFiService& wifiService);

    void showStartupScreen();
    void showDashboardScreen();
    void showWeatherScreen();
    void showAircraftScreen();
    void showSatelliteScreen();
    void showSolarScreen();
    void showLiveSpotsScreen();
    void showSettingsScreen();
    void showPotaScreen();

    WeatherScreen& getWeatherScreen();

private:
    void handleNavigation(
        Page page);

    static void startupTimerCallback(
        lv_timer_t* timer);

    StartupScreen startupScreen;
    DashboardScreen dashboardScreen;
    WeatherScreen weatherScreen;
    AircraftScreen aircraftScreen;
    SatelliteScreen satelliteScreen;
    SolarScreen solarScreen;
    LiveSpotsScreen liveSpotsScreen;
    SettingsScreen settingsScreen;
    PotaScreen potaScreen;

    lv_timer_t* startupTimer = nullptr;
};
