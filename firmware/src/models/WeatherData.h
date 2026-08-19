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

    bool airQualityValid = false;
    int16_t usAqi = 0;
    float pm25 = NAN;
    float pm10 = NAN;

    String windDirection = "--";
    String stationId = "--";
    String observationTime;

    unsigned long lastSuccessfulUpdateMs = 0;
};
