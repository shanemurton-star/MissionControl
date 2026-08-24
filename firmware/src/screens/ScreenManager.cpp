#include "ScreenManager.h"
#include <esp_heap_caps.h>

namespace
{
    void logScreenMemory(const char* screenName)
    {
        Serial.print("[Memory] after ");
        Serial.print(screenName);
        Serial.print(" internal free=");
        Serial.print(heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
        Serial.print(" largest=");
        Serial.print(heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL));
        Serial.print(" PSRAM free=");
        Serial.println(heap_caps_get_free_size(MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    }
}

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
    this->clockService = &clockService;
    this->weatherService = &weatherService;
    this->aircraftService = &aircraftService;
    this->satelliteService = &satelliteService;
    this->solarService = &solarService;
    this->liveSpotsService = &liveSpotsService;
    this->settingsService = &settingsService;
    this->wifiService = &wifiService;
    this->potaService = &potaService;

    HeaderBar::configureSettings(settingsService.get());
    Serial.println("[UI] Startup screen...");
    startupScreen.begin();
    logScreenMemory("startup screen");
    Serial.println("[UI] Dashboard screen...");

    dashboardScreen.begin(
        clockService,
        weatherService,
        aircraftService,
        satelliteService,
        solarService,
        liveSpotsService,
        potaService);
    logScreenMemory("dashboard screen");
    Serial.println("[UI] Detail screens will be created on demand.");

    /*
     * Central navigation callbacks
     */
    dashboardScreen.setNavigationCallback(
        [this](Page page)
        {
            handleNavigation(page);
        });

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
    weatherScreen.begin(*clockService, *weatherService);
    weatherScreen.setNavigationCallback([this](Page page) { handleNavigation(page); });
    weatherScreen.show();
}

void ScreenManager::showAircraftScreen()
{
    aircraftScreen.begin(*clockService, *aircraftService);
    aircraftScreen.setNavigationCallback([this](Page page) { handleNavigation(page); });
    aircraftScreen.show();
}

void ScreenManager::showSatelliteScreen()
{
    satelliteScreen.begin(*clockService, *satelliteService);
    satelliteScreen.setNavigationCallback([this](Page page) { handleNavigation(page); });
    satelliteScreen.show();
}
void ScreenManager::showSolarScreen()
{
    solarScreen.begin(*clockService, *solarService);
    solarScreen.setNavigationCallback([this](Page page) { handleNavigation(page); });
    solarScreen.show();
}
void ScreenManager::showLiveSpotsScreen()
{
    liveSpotsScreen.begin(*clockService, *liveSpotsService);
    liveSpotsScreen.setNavigationCallback([this](Page page) { handleNavigation(page); });
    liveSpotsScreen.show();
}
void ScreenManager::showSettingsScreen()
{
    settingsScreen.begin(*clockService, *settingsService, *wifiService, *liveSpotsService);
    settingsScreen.setNavigationCallback([this](Page page) { handleNavigation(page); });
    settingsScreen.show();
}
void ScreenManager::showPotaScreen()
{
    potaScreen.begin(*clockService, *potaService);
    potaScreen.setNavigationCallback([this](Page page) { handleNavigation(page); });
    potaScreen.show();
}

WeatherScreen& ScreenManager::getWeatherScreen()
{
    return weatherScreen;
}

void ScreenManager::handleNavigation(
    Page page)
{
    const Page previousPage = currentPage;
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

    currentPage = page;
    if (previousPage != page) releaseDetailScreen(previousPage);
}

void ScreenManager::releaseDetailScreen(Page page)
{
    switch (page)
    {
        case Page::Weather: weatherScreen.release(); break;
        case Page::Aircraft: aircraftScreen.release(); break;
        case Page::Satellite: satelliteScreen.release(); break;
        case Page::Solar: solarScreen.release(); break;
        case Page::LiveSpots: liveSpotsScreen.release(); break;
        case Page::Settings: settingsScreen.release(); break;
        case Page::Pota: potaScreen.release(); break;
        default: break;
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

    static const Page DEFAULT_SCREEN_PAGES[7] = {
        Page::Dashboard,
        Page::Weather,
        Page::Aircraft,
        Page::Satellite,
        Page::Solar,
        Page::LiveSpots,
        Page::Pota
    };
    uint8_t selection = manager->settingsService != nullptr
        ? manager->settingsService->get().defaultScreen
        : 0;
    if (selection > 6) selection = 0;
    manager->handleNavigation(DEFAULT_SCREEN_PAGES[selection]);
    manager->startupTimer = nullptr;
}
