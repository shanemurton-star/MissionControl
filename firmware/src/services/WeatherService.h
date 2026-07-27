#pragma once

#include <Arduino.h>

#include "../models/WeatherData.h"
#include "../models/AppSettings.h"

class WeatherService
{
public:
    void begin(const AppSettings& settings);
    void update();

    bool isValid() const;
    bool isUpdating() const;

    const WeatherData& getCurrentWeather() const;
    const String& getLastError() const;

    String getTemperature() const;
    String getHumidity() const;
    String getCondition() const;
    String getWind() const;
    String getPressure() const;
    String getDewPoint() const;
    String getStationId() const;
    String getObservationTime() const;

private:
    enum class State
    {
        WaitingForWiFi,
        ResolvePoint,
        ResolveStation,
        FetchObservation,
        Ready,
        RetryDelay
    };

    static constexpr unsigned long RETRY_INTERVAL_MS =
        60UL * 1000UL;

    static constexpr unsigned long REQUEST_TIMEOUT_MS =
        15000UL;

    void resolvePoint();
    void resolveStation();
    void fetchObservation();

    bool performRequest(
        const String& url,
        String& response);

    void setError(
        const String& message);

    void scheduleRetry();

    static float celsiusToFahrenheit(
        float celsius);

    static float kilometersPerHourToMph(
        float kilometersPerHour);

    static float metersPerSecondToMph(
        float metersPerSecond);

    static float pascalsToInHg(
        float pascals);

    static String degreesToCompass(
        float degrees);

    static String extractStationId(
        const String& stationUrl);

    static bool timeReached(
        unsigned long targetTime);

    double latitude = 0.0;
    double longitude = 0.0;

    unsigned long refreshIntervalMs =
    10UL * 60UL * 1000UL;

    State state = State::WaitingForWiFi;

    WeatherData currentWeather;

    String stationsUrl;
    String latestObservationUrl;
    String lastError;

    unsigned long nextActionMs = 0;

    bool updating = false;
};