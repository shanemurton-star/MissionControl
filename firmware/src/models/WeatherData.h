#pragma once

#include <Arduino.h>

struct WeatherData
{
    bool valid = false;

    String condition =
        "Waiting for weather";

    String iconUrl;

    float temperatureF = NAN;
    float humidityPercent = NAN;
    float dewPointF = NAN;
    float windSpeedMph = NAN;
    float windGustMph = NAN;
    float pressureInHg = NAN;

    String windDirection = "--";
    String stationId = "--";
    String observationTime;

    unsigned long lastSuccessfulUpdateMs = 0;
};