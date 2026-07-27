#pragma once

#include <lvgl.h>

#include "../services/ClockService.h"
#include "../services/WeatherService.h"
#include "../ui/NavigationBar.h"

class DashboardScreen
{
public:
    void begin(
    ClockService& clockService,
    WeatherService& weatherService);
    void show();
    NavigationBar& getNavigationBar();

private:
    static void updateTimerCallback(lv_timer_t* timer);

    void update();

    lv_obj_t* screen = nullptr;

    // Header
    lv_obj_t* localTimeLabel = nullptr;
    lv_obj_t* utcTimeLabel = nullptr;

    lv_obj_t* wifiStatusLabel = nullptr;
    lv_obj_t* ntpStatusLabel = nullptr;
    

    // Weather panel
    lv_obj_t* weatherConditionLabel = nullptr;
    lv_obj_t* weatherTempLabel = nullptr;
    lv_obj_t* weatherDetailsLabel = nullptr;

     // Navigation
    NavigationBar navigationBar;

    lv_timer_t* updateTimer = nullptr;

    ClockService* clockService = nullptr;
    WeatherService* weatherService = nullptr;
};