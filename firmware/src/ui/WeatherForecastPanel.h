#pragma once

#include <lvgl.h>

#include "../services/WeatherService.h"

class WeatherForecastPanel
{
public:
    void create(
        lv_obj_t* parent,
        WeatherService& weatherService,
        int16_t x,
        int16_t y,
        int16_t width,
        int16_t height);

    void update();

private:
    static String shorten(
        const String& value,
        size_t maximumLength);

    WeatherService* weatherService = nullptr;

    lv_obj_t* panel = nullptr;
    lv_obj_t* contentLabel = nullptr;
};
