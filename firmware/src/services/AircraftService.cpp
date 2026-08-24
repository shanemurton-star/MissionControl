#include "AircraftService.h"

#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <math.h>
#include <time.h>

namespace
{
    constexpr double EARTH_RADIUS_NM = 3440.065;

    String cleanString(const char* value)
    {
        String result = value == nullptr ? "" : value;
        result.trim();
        return result;
    }
}

void AircraftService::begin(const AppSettings& settings)
{
    latitude = settings.latitude;
    longitude = settings.longitude;
    requestUrl = String("https://opendata.adsb.fi/api/v3/lat/") +
        String(latitude, 5) + "/lon/" + String(longitude, 5) + "/dist/" +
        String(SEARCH_RADIUS_NM, 0);
    nextActionMs = 0;
}

void AircraftService::update()
{
    if (!timeReached(nextActionMs) || updating || WiFi.status() != WL_CONNECTED)
    {
        return;
    }
    fetchAircraft();
}

void AircraftService::fetchAircraft()
{
    updating = true;
    lastError = "";

    WiFiClientSecure secureClient;
    secureClient.setInsecure();

    HTTPClient http;
    http.setConnectTimeout(REQUEST_TIMEOUT_MS);
    http.setTimeout(REQUEST_TIMEOUT_MS);
    http.setUserAgent("MissionControl-ESP32/1.0");

    if (!http.begin(secureClient, requestUrl))
    {
        lastError = "Unable to start aircraft request";
        updating = false;
        nextActionMs = millis() + RETRY_INTERVAL_MS;
        return;
    }

    const int responseCode = http.GET();
    if (responseCode < 200 || responseCode >= 300)
    {
        lastError = responseCode < 0
            ? String("Aircraft connection: ") + HTTPClient::errorToString(responseCode)
            : String("Aircraft API HTTP ") + responseCode;
        http.end();
        updating = false;
        nextActionMs = millis() + RETRY_INTERVAL_MS;
        return;
    }

    JsonDocument filter;
    JsonObject itemFilter = filter["ac"].add<JsonObject>();
    itemFilter["hex"] = true;
    itemFilter["flight"] = true;
    itemFilter["r"] = true;
    itemFilter["t"] = true;
    itemFilter["desc"] = true;
    itemFilter["category"] = true;
    itemFilter["emergency"] = true;
    itemFilter["lat"] = true;
    itemFilter["lon"] = true;
    itemFilter["alt_baro"] = true;
    itemFilter["gs"] = true;
    itemFilter["ias"] = true;
    itemFilter["mach"] = true;
    itemFilter["track"] = true;
    itemFilter["baro_rate"] = true;
    itemFilter["nav_altitude_mcp"] = true;
    itemFilter["nav_heading"] = true;
    itemFilter["squawk"] = true;
    itemFilter["seen_pos"] = true;

    JsonDocument document;
    const DeserializationError error = deserializeJson(
        document,
        http.getStream(),
        DeserializationOption::Filter(filter));
    http.end();

    if (error)
    {
        lastError = String("Aircraft data error: ") + error.c_str();
        updating = false;
        nextActionMs = millis() + RETRY_INTERVAL_MS;
        return;
    }

    aircraftCount = 0;
    for (JsonObject source : document["ac"].as<JsonArray>())
    {
        if (aircraftCount >= MAX_AIRCRAFT ||
            !source["lat"].is<double>() || !source["lon"].is<double>())
        {
            continue;
        }

        AircraftData& target = aircraft[aircraftCount++];
        target = AircraftData{};
        target.hex = cleanString(source["hex"] | "");
        target.callsign = cleanString(source["flight"] | "");
        target.registration = cleanString(source["r"] | "");
        target.type = cleanString(source["t"] | "");
        target.description = cleanString(source["desc"] | "");
        target.category = cleanString(source["category"] | "");
        target.emergency = cleanString(source["emergency"] | "");
        target.squawk = cleanString(source["squawk"] | "");
        target.latitude = source["lat"] | 0.0;
        target.longitude = source["lon"] | 0.0;
        target.groundSpeedKnots = source["gs"] | 0.0f;
        target.indicatedSpeedKnots = source["ias"] | 0.0f;
        target.mach = source["mach"] | 0.0f;
        target.trackDegrees = source["track"] | 0.0f;
        target.verticalRateFpm = source["baro_rate"] | 0;
        target.selectedAltitudeFeet = source["nav_altitude_mcp"] | 0.0f;
        target.selectedHeadingDegrees = source["nav_heading"] | 0.0f;
        target.seenSeconds = source["seen_pos"] | 0.0f;

        if (source["alt_baro"].is<const char*>())
        {
            target.onGround = strcmp(source["alt_baro"], "ground") == 0;
        }
        else
        {
            target.altitudeFeet = source["alt_baro"] | 0.0f;
        }

        calculatePosition(target);
    }

    sortByDistance();
    valid = true;
    updating = false;
    lastUpdateTime = static_cast<uint32_t>(time(nullptr));
    nextActionMs = millis() + REFRESH_INTERVAL_MS;

    Serial.print("[AircraftService] Tracking ");
    Serial.print(aircraftCount);
    Serial.println(" aircraft within 25 nm");
}

void AircraftService::calculatePosition(AircraftData& target) const
{
    const double lat1 = latitude * DEG_TO_RAD;
    const double lat2 = target.latitude * DEG_TO_RAD;
    const double deltaLat = (target.latitude - latitude) * DEG_TO_RAD;
    const double deltaLon = (target.longitude - longitude) * DEG_TO_RAD;
    const double a = sin(deltaLat / 2.0) * sin(deltaLat / 2.0) +
        cos(lat1) * cos(lat2) * sin(deltaLon / 2.0) * sin(deltaLon / 2.0);
    target.distanceNm = static_cast<float>(
        EARTH_RADIUS_NM * 2.0 * atan2(sqrt(a), sqrt(1.0 - a)));

    const double y = sin(deltaLon) * cos(lat2);
    const double x = cos(lat1) * sin(lat2) -
        sin(lat1) * cos(lat2) * cos(deltaLon);
    target.bearingDegrees = static_cast<float>(fmod(atan2(y, x) / DEG_TO_RAD + 360.0, 360.0));
}

void AircraftService::sortByDistance()
{
    for (uint8_t i = 1; i < aircraftCount; ++i)
    {
        AircraftData value = aircraft[i];
        int8_t position = i - 1;
        while (position >= 0 && aircraft[position].distanceNm > value.distanceNm)
        {
            aircraft[position + 1] = aircraft[position];
            --position;
        }
        aircraft[position + 1] = value;
    }
}

bool AircraftService::isValid() const { return valid; }
bool AircraftService::isUpdating() const { return updating; }
uint8_t AircraftService::getAircraftCount() const { return aircraftCount; }
const AircraftData& AircraftService::getAircraft(uint8_t index) const { return aircraft[index]; }
uint32_t AircraftService::getLastUpdateTime() const { return lastUpdateTime; }
const String& AircraftService::getLastError() const { return lastError; }
double AircraftService::getCenterLatitude() const { return latitude; }
double AircraftService::getCenterLongitude() const { return longitude; }

bool AircraftService::timeReached(unsigned long targetTime)
{
    return static_cast<long>(millis() - targetTime) >= 0;
}
