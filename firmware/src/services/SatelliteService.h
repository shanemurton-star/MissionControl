#pragma once

#include <Arduino.h>
#include <Sgp4.h>

#include "../models/AppSettings.h"
#include "../models/SatelliteData.h"

class SatelliteService
{
public:
    static constexpr uint8_t SATELLITE_COUNT = 5;

    void begin(const AppSettings& settings);
    void update();

    bool isValid() const;
    bool isUpdating() const;
    const SatelliteData& getSatellite(uint8_t index) const;
    const SatelliteData* getNextPass() const;
    const String& getLastError() const;
    void requestRadioData(uint8_t index);

private:
    static constexpr unsigned long TLE_REFRESH_MS = 2UL * 60UL * 60UL * 1000UL;
    static constexpr unsigned long RETRY_MS = 5UL * 60UL * 1000UL;
    static constexpr unsigned long POSITION_UPDATE_MS = 1000UL;
    static constexpr unsigned long REQUEST_TIMEOUT_MS = 15000UL;

    void fetchNextTle();
    bool fetchTle(uint8_t index);
    bool fetchRadioData(uint8_t index);
    void calculatePasses();
    void updatePositions();
    static uint32_t julianToUnix(double julianDay);
    static bool timeReached(unsigned long targetTime);

    Sgp4 predictors[SATELLITE_COUNT];
    SatelliteData satellites[SATELLITE_COUNT];
    double latitude = 0.0;
    double longitude = 0.0;
    uint8_t fetchIndex = 0;
    unsigned long nextTleRefreshMs = 0;
    unsigned long nextPositionUpdateMs = 0;
    bool valid = false;
    bool updating = false;
    int8_t pendingRadioIndex = -1;
    String lastError;
};
