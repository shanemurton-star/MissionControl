#pragma once

#include <Arduino.h>

struct SatelliteRadioChannel
{
    String description;
    String mode;
    String service;
    uint64_t uplinkHz = 0;
    uint64_t downlinkHz = 0;
    uint32_t baud = 0;
};

struct SatelliteData
{
    static constexpr uint8_t MAX_RADIO_CHANNELS = 8;

    String name;
    uint32_t catalogNumber = 0;
    uint32_t aosTime = 0;
    uint32_t maxTime = 0;
    uint32_t losTime = 0;
    float maxElevation = 0.0f;
    float aosAzimuth = 0.0f;
    float losAzimuth = 0.0f;
    float currentAzimuth = 0.0f;
    float currentElevation = -90.0f;
    float rangeKm = 0.0f;
    bool visible = false;
    bool valid = false;
    bool radioReady = false;
    uint8_t radioChannelCount = 0;
    SatelliteRadioChannel radioChannels[MAX_RADIO_CHANNELS];
    String radioError;
};
