#pragma once

#include <Arduino.h>

struct ForecastPeriod
{
    String name;
    String shortForecast;
    int16_t temperatureF = 0;
    bool daytime = false;
};

struct HourlyForecastPeriod
{
    String startTime;
    String shortForecast;
    String windSpeed;
    String windDirection;
    int16_t temperatureF = 0;
    int16_t precipitationPercent = 0;
};

struct ForecastData
{
    // The NWS normally supplies seven daytime and seven nighttime periods.
    static constexpr size_t MAX_PERIODS = 14;
    static constexpr size_t MAX_HOURLY_PERIODS = 12;

    bool valid = false;
    ForecastPeriod periods[MAX_PERIODS];
    size_t periodCount = 0;

    bool hourlyValid = false;
    HourlyForecastPeriod hourlyPeriods[MAX_HOURLY_PERIODS];
    size_t hourlyPeriodCount = 0;

    bool hasHigh = false;
    bool hasLow = false;
    int16_t highF = 0;
    int16_t lowF = 0;

    size_t alertCount = 0;
    String primaryAlert;
};
