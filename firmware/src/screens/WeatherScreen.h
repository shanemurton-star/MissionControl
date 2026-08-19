#pragma once

#include <functional>

#include <lvgl.h>

#include "../models/Page.h"
#include "../services/ClockService.h"
#include "../services/RadarService.h"
#include "../services/WeatherService.h"
#include "../ui/HeaderBar.h"
#include "../ui/WeatherForecastPanel.h"
#include "../ui/WeatherRadarPanel.h"
#include "../ui/WeatherPanel.h"

class WeatherScreen
{
public:
    using NavigationCallback =
        std::function<void(Page)>;

    void begin(
        ClockService& clockService,
        WeatherService& weatherService,
        RadarService& radarService);

    void show();

    void setNavigationCallback(
        NavigationCallback callback);

private:
    static void backButtonEventHandler(
        lv_event_t* event);

    static void updateTimerCallback(
        lv_timer_t* timer);

    void update();

    lv_obj_t* screen = nullptr;

    HeaderBar headerBar;
    WeatherRadarPanel radarPanel;
    WeatherPanel currentConditionsPanel;
    WeatherForecastPanel forecastPanel;

    NavigationCallback navigationCallback;

    lv_timer_t* updateTimer = nullptr;

    ClockService* clockService = nullptr;
    WeatherService* weatherService = nullptr;
    RadarService* radarService = nullptr;
};
