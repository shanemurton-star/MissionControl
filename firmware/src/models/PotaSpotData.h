#pragma once

#include <Arduino.h>

struct PotaSpotData
{
    String activator;
    String frequency;
    String mode;
    String reference;
    String name;
    String location;
    double latitude = 0.0;
    double longitude = 0.0;
    float distanceMiles = 0.0f;
    float bearingDegrees = 0.0f;
};
