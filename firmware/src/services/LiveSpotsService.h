#pragma once
#include <Arduino.h>
#include "../models/AppSettings.h"
#include "../models/LiveSpotData.h"

class LiveSpotsService
{
public:
    static constexpr uint8_t BAND_COUNT = 9;
    static constexpr uint8_t MAX_RECENT_SPOTS = 12;
    void begin(const AppSettings& settings);
    void update();
    bool isValid() const;
    bool isUpdating() const;
    const BandSpotSummary& getBand(uint8_t index) const;
    const LiveSpot& getRecentSpot(uint8_t index) const;
    uint8_t getRecentSpotCount() const;
    uint16_t getTotalSpotCount() const;
    const String& getLastError() const;
    const String& getGridSquare() const;

private:
    static constexpr unsigned long REFRESH_MS = 5UL * 60UL * 1000UL;
    static constexpr unsigned long RETRY_MS = 60UL * 1000UL;
    static constexpr unsigned long REQUEST_TIMEOUT_MS = 20000UL;
    bool fetchReports();
    void parseReports(const String& xml);
    static String attribute(const String& tag, const char* name);
    static int8_t bandIndex(uint32_t frequencyHz);
    static bool gridToCoordinates(const String& grid, double& latitude, double& longitude);
    float distanceFromHome(const String& grid) const;
    static bool timeReached(unsigned long target);

    BandSpotSummary bands[BAND_COUNT];
    LiveSpot recentSpots[MAX_RECENT_SPOTS];
    uint8_t recentSpotCount = 0;
    uint16_t totalSpotCount = 0;
    String gridSquare;
    String queryGrid;
    String lastError;
    double homeLatitude = 0.0;
    double homeLongitude = 0.0;
    unsigned long nextUpdateMs = 0;
    bool valid = false;
    bool updating = false;
};
