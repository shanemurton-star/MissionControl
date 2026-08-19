#pragma once

#include <Arduino.h>

struct ForecastPeriod
{
    String name;
    String shortForecast;
    int16_t temperatureF = 0;
    bool daytime = false;
};

struct ForecastData
{
    // The NWS normally supplies seven daytime and seven nighttime periods.
    static constexpr size_t MAX_PERIODS = 14;

    bool valid = false;
    ForecastPeriod periods[MAX_PERIODS];
    size_t periodCount = 0;

    bool hasHigh = false;
    bool hasLow = false;
    int16_t highF = 0;
    int16_t lowF = 0;

    size_t alertCount = 0;
    String primaryAlert;
};
