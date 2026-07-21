#pragma once

#include <Arduino.h>

#include "StatusService.h"


class WeatherService
{
public:

    void begin();

    void update(
        StatusService& status
    );

    String getTemperature();

    String getConditions();

    String getHumidity();

    String getWind();


private:

    String temperature = "--";
    String conditions = "UNKNOWN";
    String humidity = "--";
    String wind = "--";
};