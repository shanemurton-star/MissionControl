#pragma once

#include <Arduino.h>
#include <Preferences.h>
#include <Sgp4.h>

#include "../models/AppSettings.h"
#include "../models/SatelliteData.h"

class SatelliteService
{
public:
    static constexpr uint8_t SATELLITE_COUNT = 12;

    void begin(const AppSettings& settings);
    void update();

    bool isValid() const;
    bool isUpdating() const;
    const SatelliteData& getSatellite(uint8_t index) const;
    const SatelliteData* getNextPass() const;
    const String& getLastError() const;
    bool isUsingCachedData() const;
    void requestRadioData(uint8_t index);

private:
    static constexpr unsigned long TLE_REFRESH_MS = 2UL * 60UL * 60UL * 1000UL;
    static constexpr unsigned long INITIAL_TLE_REFRESH_DELAY_MS = 60UL * 1000UL;
    static constexpr unsigned long TLE_FETCH_GAP_MS = 5UL * 1000UL;
    static constexpr unsigned long SOURCE_FAILURE_BACKOFF_MS =
        2UL * 60UL * 60UL * 1000UL;
    static constexpr unsigned long RETRY_MS = 30UL * 1000UL;
    static constexpr unsigned long POSITION_UPDATE_MS = 1000UL;
    static constexpr unsigned long REQUEST_TIMEOUT_MS = 8000UL;
    static constexpr uint32_t MAX_CACHE_AGE_SECONDS = 7UL * 24UL * 60UL * 60UL;

    void fetchNextTle();
    bool fetchTle(uint8_t index);
    bool fetchRadioData(uint8_t index);
    bool initializeTle(
        uint8_t index, const String& name,
        const String& line1, const String& line2);
    void loadCachedTles();
    void saveCachedTle(
        uint8_t index, const String& name,
        const String& line1, const String& line2);
    bool activateCachedTles();
    void calculatePasses();
    void updatePositions();
    static uint32_t julianToUnix(double julianDay);
    static bool timeReached(unsigned long targetTime);

    Sgp4 predictors[SATELLITE_COUNT];
    SatelliteData satellites[SATELLITE_COUNT];
    bool tleLoaded[SATELLITE_COUNT] = {};
    bool tleFromCache[SATELLITE_COUNT] = {};
    uint32_t tleUpdatedAt[SATELLITE_COUNT] = {};
    Preferences cachePreferences;
    double latitude = 0.0;
    double longitude = 0.0;
    uint8_t fetchIndex = 0;
    unsigned long nextTleRefreshMs = 0;
    unsigned long nextPositionUpdateMs = 0;
    bool valid = false;
    bool updating = false;
    bool preferCelestrak = false;
    bool cacheReady = false;
    bool cacheEvaluated = false;
    bool usingCachedData = false;
    int8_t pendingRadioIndex = -1;
    String lastError;
};
