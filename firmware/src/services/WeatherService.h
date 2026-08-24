#pragma once

#include <Arduino.h>

#include "../models/WeatherData.h"
#include "../models/ForecastData.h"
#include "../models/AppSettings.h"

class WeatherService
{
public:
    void begin(const AppSettings& settings);
    void update();

    bool isValid() const;
    bool isUpdating() const;
    uint32_t getDataRevision() const;

    const WeatherData& getCurrentWeather() const;
    const ForecastData& getForecast() const;
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
        FetchForecast,
        FetchAlerts,
        FetchAirQuality,
        Ready,
        RetryDelay
    };

    static constexpr unsigned long RETRY_INTERVAL_MS =
        15UL * 1000UL;

    static constexpr unsigned long REQUEST_TIMEOUT_MS =
        8000UL;

    void resolvePoint();
    void resolveStation();
    void fetchObservation();
    void fetchForecast();
    void fetchAlerts();
    void fetchAirQuality();

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
    ForecastData forecast;

    String stationsUrl;
    String latestObservationUrl;
    String forecastUrl;
    String alertsUrl;
    String lastError;

    unsigned long nextActionMs = 0;
    uint32_t dataRevision = 0;

    bool updating = false;
};
