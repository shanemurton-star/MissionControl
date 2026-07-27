#pragma once

#include <lvgl.h>

#include "DashboardScreen.h"
#include "StartupScreen.h"
#include "WeatherScreen.h"

#include "../services/ClockService.h"
#include "../services/WeatherService.h"

class ScreenManager
{
public:
    void begin(
        ClockService& clockService,
        WeatherService& weatherService);

    void showStartupScreen();
    void showDashboardScreen();
    void showWeatherScreen();
    WeatherScreen& getWeatherScreen();

private:
    static void startupTimerCallback(
        lv_timer_t* timer);

    StartupScreen startupScreen;
    DashboardScreen dashboardScreen;
    WeatherScreen weatherScreen;

    lv_timer_t* startupTimer = nullptr;
};