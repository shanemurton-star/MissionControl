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
    float distanceMiles = 0.0f;
};
