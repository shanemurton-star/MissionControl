#pragma once

#include <functional>

#include <lvgl.h>

#include "../services/WeatherService.h"

class WeatherPanel
{
public:
    using ClickCallback = std::function<void()>;

    void create(
        lv_obj_t* parent,
        WeatherService& weatherService,
        int16_t x,
        int16_t y,
        int16_t width,
        int16_t height);

    void update();

    void setClickCallback(
        ClickCallback callback);

private:
    static void panelEventHandler(
        lv_event_t* event);

    ClickCallback clickCallback;

    WeatherService* weatherService = nullptr;

    lv_obj_t* panel = nullptr;
    lv_obj_t* weatherIcon = nullptr;
    lv_obj_t* conditionLabel = nullptr;
    lv_obj_t* temperatureLabel = nullptr;
    lv_obj_t* detailsLabel = nullptr;
};
