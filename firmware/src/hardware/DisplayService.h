#pragma once

#include <Arduino.h>

#include "../screens/ScreenManager.h"
#include "../services/ClockService.h"
#include "../services/WeatherService.h"

class DisplayService
{
public:
    bool begin(
        ClockService& clockService,
        WeatherService& weatherService);

    void update();

private:
    uint32_t lastTickMillis = 0;

    ScreenManager screenManager;
};