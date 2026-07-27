#pragma once

#include <lvgl.h>

#include "../services/ClockService.h"
#include "../services/WeatherService.h"
#include "../ui/NavigationBar.h"

class WeatherScreen
{
public:
    void begin(
        ClockService& clockService,
        WeatherService& weatherService);

    void show();

    NavigationBar& getNavigationBar();

private:
    void update();

    lv_obj_t* screen = nullptr;

    NavigationBar navigationBar;

    ClockService* clockService = nullptr;
    WeatherService* weatherService = nullptr;
};