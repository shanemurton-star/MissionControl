#include "ScreenManager.h"

void ScreenManager::begin(
    ClockService& clockService,
    WeatherService& weatherService,
    AircraftService& aircraftService,
    SatelliteService& satelliteService,
    SolarService& solarService,
    LiveSpotsService& liveSpotsService,
    PotaService& potaService,
    SettingsService& settingsService,
    WiFiService& wifiService)
{
    HeaderBar::configureSettings(settingsService.get());
    Serial.println("[UI] Startup screen...");
    startupScreen.begin();
    Serial.println("[UI] Dashboard screen...");

    dashboardScreen.begin(
        clockService,
        weatherService,
        aircraftService,
        satelliteService,
        solarService,
        liveSpotsService,
        potaService);
    Serial.println("[UI] Weather screen...");

    weatherScreen.begin(
        clockService,
        weatherService);

    Serial.println("[UI] Aircraft screen...");
    aircraftScreen.begin(clockService, aircraftService);
    Serial.println("[UI] Satellite screen...");
    satelliteScreen.begin(clockService, satelliteService);
    Serial.println("[UI] Solar screen...");
    solarScreen.begin(clockService, solarService);
    Serial.println("[UI] Live Spots screen...");
    liveSpotsScreen.begin(clockService, liveSpotsService);
    Serial.println("[UI] Settings screen...");
    settingsScreen.begin(clockService, settingsService, wifiService, liveSpotsService);
    Serial.println("[UI] POTA screen...");
    potaScreen.begin(clockService, potaService);
    Serial.println("[UI] All screens created.");

    /*
     * Central navigation callbacks
     */
    dashboardScreen.setNavigationCallback(
        [this](Page page)
        {
            handleNavigation(page);
        });

    weatherScreen.setNavigationCallback(
        [this](Page page)
        {
            handleNavigation(page);
        });

    aircraftScreen.setNavigationCallback(
        [this](Page page) { handleNavigation(page); });

    satelliteScreen.setNavigationCallback(
        [this](Page page) { handleNavigation(page); });
    solarScreen.setNavigationCallback(
        [this](Page page) { handleNavigation(page); });
    liveSpotsScreen.setNavigationCallback(
        [this](Page page) { handleNavigation(page); });
    settingsScreen.setNavigationCallback(
        [this](Page page) { handleNavigation(page); });
    potaScreen.setNavigationCallback(
        [this](Page page) { handleNavigation(page); });

    showStartupScreen();

    startupTimer = lv_timer_create(
        startupTimerCallback,
        2000,
        this);

    lv_timer_set_repeat_count(
        startupTimer,
        1);
}

void ScreenManager::showStartupScreen()
{
    startupScreen.show();
}

void ScreenManager::showDashboardScreen()
{
    dashboardScreen.show();
}

void ScreenManager::showWeatherScreen()
{
    weatherScreen.show();
}

void ScreenManager::showAircraftScreen()
{
    aircraftScreen.show();
}

void ScreenManager::showSatelliteScreen()
{
    satelliteScreen.show();
}
void ScreenManager::showSolarScreen()
{
    solarScreen.show();
}
void ScreenManager::showLiveSpotsScreen()
{
    liveSpotsScreen.show();
}
void ScreenManager::showSettingsScreen()
{
    settingsScreen.show();
}
void ScreenManager::showPotaScreen()
{
    potaScreen.show();
}

WeatherScreen& ScreenManager::getWeatherScreen()
{
    return weatherScreen;
}

void ScreenManager::handleNavigation(
    Page page)
{
    switch (page)
    {
        case Page::Dashboard:
            showDashboardScreen();
            break;

        case Page::Weather:
            showWeatherScreen();
            break;

        case Page::Aircraft:
            showAircraftScreen();
            break;

        case Page::Satellite:
            showSatelliteScreen();
            break;
        case Page::Solar:
            showSolarScreen();
            break;
        case Page::LiveSpots:
            showLiveSpotsScreen();
            break;
        case Page::Settings:
            showSettingsScreen();
            break;
        case Page::Pota:
            showPotaScreen();
            break;

        /*
         * These screens will be added next.
         */
        case Page::Ham:
        case Page::System:
            break;
    }
}

void ScreenManager::startupTimerCallback(
    lv_timer_t* timer)
{
    ScreenManager* manager =
        static_cast<ScreenManager*>(
            timer->user_data);

    if (manager == nullptr)
    {
        return;
    }

    manager->showDashboardScreen();
    manager->startupTimer = nullptr;
}
