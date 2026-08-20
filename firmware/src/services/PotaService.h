#pragma once

#include <Arduino.h>

#include "../models/AppSettings.h"
#include "../models/PotaSpotData.h"

class PotaService
{
public:
    static constexpr uint8_t MAX_SPOTS = 40;
    static constexpr float ACTIVE_RADIUS_MILES = 100.0f;

    void begin(const AppSettings& settings);
    void update();
    bool isValid() const;
    bool isUpdating() const;
    uint8_t getSpotCount() const;
    const PotaSpotData& getSpot(uint8_t index) const;
    const String& getLastError() const;

private:
    static constexpr uint32_t REFRESH_MS = 15UL * 60UL * 1000UL;
    static constexpr uint32_t RETRY_MS = 30UL * 1000UL;
    static constexpr uint32_t REQUEST_TIMEOUT_MS = 8000UL;

    void fetchSpots();
    void insertSpot(const PotaSpotData& spot);
    float distanceMiles(double latitude, double longitude) const;
    static bool timeReached(uint32_t target);

    double homeLatitude = 0.0;
    double homeLongitude = 0.0;
    PotaSpotData spots[MAX_SPOTS];
    uint8_t spotCount = 0;
    uint32_t nextUpdateMs = 0;
    bool valid = false;
    bool updating = false;
    String lastError;
};
