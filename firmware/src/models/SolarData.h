#pragma once

#include <Arduino.h>

struct SolarData
{
    float solarFlux = 0.0f;
    float kpIndex = 0.0f;
    int aIndex = 0;
    String xrayClass;
    String flarePeakClass;
    float solarWindSpeed = 0.0f;
    float magneticField = 0.0f;
    int radioBlackoutScale = 0;
    int solarRadiationScale = 0;
    int geomagneticStormScale = 0;
    uint32_t updatedAt = 0;
};
