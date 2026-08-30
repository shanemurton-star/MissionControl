#pragma once

#include <Arduino.h>

struct AircraftData
{
    String hex;
    String callsign;
    String registration;
    String type;
    String description;
    String category;
    String emergency;
    String squawk;
    uint8_t databaseFlags = 0;
    double latitude = 0.0;
    double longitude = 0.0;
    float distanceNm = 0.0f;
    float bearingDegrees = 0.0f;
    float altitudeFeet = 0.0f;
    float groundSpeedKnots = 0.0f;
    float indicatedSpeedKnots = 0.0f;
    float mach = 0.0f;
    float trackDegrees = 0.0f;
    int32_t verticalRateFpm = 0;
    float selectedAltitudeFeet = 0.0f;
    float selectedHeadingDegrees = 0.0f;
    float seenSeconds = 0.0f;
    bool onGround = false;
};
