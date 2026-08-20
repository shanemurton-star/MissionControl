#pragma once

#include <functional>

#include <lvgl.h>

#include "../models/Page.h"
#include "../services/ClockService.h"
#include "../services/WeatherService.h"
#include "../ui/HeaderBar.h"
#include "../ui/WeatherForecastPanel.h"

class WeatherScreen
{
public:
    using NavigationCallback =
        std::function<void(Page)>;

    void begin(
        ClockService& clockService,
        WeatherService& weatherService);

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
    WeatherForecastPanel forecastPanel;

    lv_obj_t* weatherIcon = nullptr;
    lv_obj_t* temperatureLabel = nullptr;
    lv_obj_t* conditionLabel = nullptr;
    lv_obj_t* conditionsDetailLabel = nullptr;
    lv_obj_t* airQualityLabel = nullptr;
    lv_obj_t* airQualityCategoryLabel = nullptr;
    lv_obj_t* atmosphereDetailLabel = nullptr;

    NavigationCallback navigationCallback;

    lv_timer_t* updateTimer = nullptr;

    ClockService* clockService = nullptr;
    WeatherService* weatherService = nullptr;
};
