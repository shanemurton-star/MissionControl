#pragma once

#include <functional>

#include <lvgl.h>

#include "../models/Page.h"
#include "../services/ClockService.h"
#include "../services/WeatherService.h"
#include "../ui/HeaderBar.h"

class WeatherScreen
{
public:
    using NavigationCallback =
        std::function<void(Page)>;

    void begin(
        ClockService& clockService,
        WeatherService& weatherService);

    void show();
    void release();

    void setNavigationCallback(
        NavigationCallback callback);

private:
    static void updateTimerCallback(
        lv_timer_t* timer);
    static void hourlyForecastEventHandler(lv_event_t* event);
    static void hourlyBackEventHandler(lv_event_t* event);

    void update();
    void updateAstronomy();
    void createHourlyScreen();
    void showHourlyScreen();
    void updateHourlyForecast();

    lv_obj_t* screen = nullptr;
    lv_obj_t* hourlyScreen = nullptr;

    HeaderBar headerBar;
    HeaderBar hourlyHeaderBar;
    lv_obj_t* temperatureArc = nullptr;
    lv_obj_t* windArc = nullptr;
    lv_obj_t* humidityArc = nullptr;
    lv_obj_t* airQualityArc = nullptr;
    lv_obj_t* weatherIcon = nullptr;
    lv_obj_t* temperatureLabel = nullptr;
    lv_obj_t* conditionLabel = nullptr;
    lv_obj_t* highLowLabel = nullptr;
    lv_obj_t* windDirectionLabel = nullptr;
    lv_obj_t* windSpeedLabel = nullptr;
    lv_obj_t* windGustLabel = nullptr;
    lv_obj_t* humidityLabel = nullptr;
    lv_obj_t* airQualityLabel = nullptr;
    lv_obj_t* airQualityCategoryLabel = nullptr;
    lv_obj_t* atmosphereDetailLabel = nullptr;
    lv_obj_t* dailyForecastLabels[2] = {};
    lv_obj_t* dailyHighLabels[2] = {};
    lv_obj_t* dailyLowLabels[2] = {};
    lv_obj_t* dailyForecastIcons[2] = {};
    lv_obj_t* dailyConditionLabels[2] = {};
    lv_obj_t* alertLabel = nullptr;
    lv_obj_t* hourlyForecastTarget = nullptr;
    lv_obj_t* hourlyScroller = nullptr;
    lv_obj_t* hourlyStatusLabel = nullptr;
    lv_obj_t* hourlyCards[ForecastData::MAX_HOURLY_PERIODS] = {};
    lv_obj_t* hourlyTimeLabels[ForecastData::MAX_HOURLY_PERIODS] = {};
    lv_obj_t* hourlyIcons[ForecastData::MAX_HOURLY_PERIODS] = {};
    lv_obj_t* hourlyTemperatureLabels[ForecastData::MAX_HOURLY_PERIODS] = {};
    lv_obj_t* hourlyConditionLabels[ForecastData::MAX_HOURLY_PERIODS] = {};
    lv_obj_t* hourlyPrecipitationLabels[ForecastData::MAX_HOURLY_PERIODS] = {};
    lv_obj_t* hourlyWindLabels[ForecastData::MAX_HOURLY_PERIODS] = {};

    NavigationCallback navigationCallback;

    lv_timer_t* updateTimer = nullptr;

    uint32_t renderedWeatherRevision = UINT32_MAX;
    bool renderedWeatherValid = false;
    bool renderedWeatherUpdating = false;
    uint32_t renderedHourlyRevision = UINT32_MAX;
    String renderedAstronomyDate;

    ClockService* clockService = nullptr;
    WeatherService* weatherService = nullptr;
};
