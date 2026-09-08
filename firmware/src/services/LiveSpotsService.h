#pragma once
#include <Arduino.h>
#include "../models/AppSettings.h"
#include "../models/LiveSpotData.h"

class LiveSpotsService
{
public:
    static constexpr uint8_t BAND_COUNT = 10;
    static constexpr uint8_t MAX_RECENT_SPOTS = 12;
    static constexpr uint8_t MAX_SIGNAL_SPOTS = 48;
    void begin(const AppSettings& settings);
    void update();
    bool isValid() const;
    bool isUpdating() const;
    const BandSpotSummary& getBand(uint8_t index) const;
    const LiveSpot& getRecentSpot(uint8_t index) const;
    uint8_t getRecentSpotCount() const;
    uint16_t getTotalSpotCount() const;
    uint32_t getDataRevision() const;
    const String& getLastError() const;
    const String& getGridSquare() const;
    const String& getCallsign() const;
    bool isSignalReachValid() const;
    const String& getSignalReachError() const;
    uint8_t getSignalReachCount() const;
    uint16_t getSignalReportCount() const;
    const SignalReachSpot& getSignalReachSpot(uint8_t index) const;
    float getSignalFarthestKm() const;
    const String& getSignalFarthestCall() const;
    uint32_t getSecondsUntilNextUpdate() const;
    double getHomeLatitude() const;
    double getHomeLongitude() const;

private:
    static constexpr unsigned long REFRESH_MS = 15UL * 60UL * 1000UL;
    static constexpr unsigned long RETRY_MS = 60UL * 1000UL;
    static constexpr unsigned long REQUEST_TIMEOUT_MS = 12000UL;
    bool fetchReports();
    bool fetchSignalReports();
    void parseReports(const String& xml);
    void parseSignalReports(const String& xml);
    static String attribute(const String& tag, const char* name);
    static int8_t bandIndex(uint32_t frequencyHz);
    static bool gridToCoordinates(const String& grid, double& latitude, double& longitude);
    float distanceFromHome(const String& grid) const;
    float bearingFromHome(const String& grid) const;
    static bool timeReached(unsigned long target);

    BandSpotSummary bands[BAND_COUNT];
    LiveSpot recentSpots[MAX_RECENT_SPOTS];
    uint8_t recentSpotCount = 0;
    uint16_t totalSpotCount = 0;
    SignalReachSpot signalSpots[MAX_SIGNAL_SPOTS];
    uint8_t signalReachCount = 0;
    uint16_t signalReportCount = 0;
    float signalFarthestKm = 0.0f;
    String signalFarthestCall;
    uint32_t dataRevision = 0;
    String gridSquare;
    String queryGrid;
    String callsign;
    String lastError;
    String signalReachError;
    double homeLatitude = 0.0;
    double homeLongitude = 0.0;
    unsigned long nextUpdateMs = 0;
    bool valid = false;
    bool signalReachValid = false;
    bool updating = false;
};
