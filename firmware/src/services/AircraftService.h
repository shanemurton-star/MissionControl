#pragma once

#include <Arduino.h>

#include "../models/AircraftData.h"
#include "../models/AppSettings.h"

class AircraftService
{
public:
    static constexpr uint8_t MAX_AIRCRAFT = 24;
    static constexpr float SEARCH_RADIUS_NM = 25.0f;

    void begin(const AppSettings& settings);
    void update();

    bool isValid() const;
    bool isUpdating() const;
    uint8_t getAircraftCount() const;
    const AircraftData& getAircraft(uint8_t index) const;
    uint32_t getLastUpdateTime() const;
    const String& getLastError() const;
    double getCenterLatitude() const;
    double getCenterLongitude() const;

private:
    static constexpr unsigned long REFRESH_INTERVAL_MS = 3UL * 60UL * 1000UL;
    static constexpr unsigned long RETRY_INTERVAL_MS = 60UL * 1000UL;
    static constexpr unsigned long REQUEST_TIMEOUT_MS = 8000UL;

    void fetchAircraft();
    void calculatePosition(AircraftData& aircraft) const;
    void sortByDistance();
    static bool timeReached(unsigned long targetTime);

    double latitude = 0.0;
    double longitude = 0.0;
    AircraftData aircraft[MAX_AIRCRAFT];
    uint8_t aircraftCount = 0;
    uint32_t lastUpdateTime = 0;
    unsigned long nextActionMs = 0;
    bool valid = false;
    bool updating = false;
    String requestUrl;
    String lastError;
};
