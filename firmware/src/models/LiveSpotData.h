#pragma once
#include <Arduino.h>

struct BandSpotSummary
{
    const char* name = "";
    uint16_t count = 0;
    float farthestKm = 0.0f;
    String farthestCall;
};

struct LiveSpot
{
    String senderCallsign;
    String senderLocator;
    String receiverCallsign;
    String mode;
    const char* band = "";
    float frequencyMhz = 0.0f;
    float distanceKm = 0.0f;
    int snr = 0;
    uint32_t timestamp = 0;
};
