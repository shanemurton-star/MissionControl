#include "ScreenManager.h"

void ScreenManager::begin(
    ClockService& clockService,
    WeatherService& weatherService)
{
    startupScreen.begin();

    dashboardScreen.begin(
        clockService,
        weatherService);
    
    weatherScreen.begin(
        clockService,
        weatherService);

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

WeatherScreen& ScreenManager::getWeatherScreen()
{
    return weatherScreen;
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