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
    requestUrl = String("https://api.adsb.lol/v2/point/") +
        String(latitude, 5) + "/" + String(longitude, 5) + "/" +
        String(SEARCH_RADIUS_NM, 0);
    fallbackUrl = String("https://opendata.adsb.fi/api/v3/lat/") +
        String(latitude, 5) + "/lon/" + String(longitude, 5) + "/dist/" +
        String(SEARCH_RADIUS_NM, 0);
    aircraftCount = 0;
    valid = false;
    updating = false;
    lastError = "";
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

    uint8_t reportedAircraft = 0;
    bool success = fetchFromSource(requestUrl, "adsb.lol", reportedAircraft);

    // A genuine zero is possible, but it is unusual around the configured
    // location. Confirm it through the independent compatible source before
    // publishing zero to the dashboard.
    if (!success || aircraftCount == 0)
    {
        const bool primarySucceeded = success;
        const String primaryError = lastError;
        uint8_t fallbackReported = 0;
        const bool fallbackSucceeded =
            fetchFromSource(fallbackUrl, "adsb.fi", fallbackReported);
        if (fallbackSucceeded)
        {
            success = true;
            reportedAircraft = fallbackReported;
        }
        else if (primarySucceeded)
        {
            // The primary source successfully confirmed an empty result.
            success = true;
            lastError = "";
        }
        else
        {
            lastError = primaryError + "; fallback: " + lastError;
        }
    }

    if (!success)
    {
        valid = false;
        updating = false;
        nextActionMs = millis() + RETRY_INTERVAL_MS;
        return;
    }

    sortByDistance();
    valid = true;
    updating = false;
    lastUpdateTime = static_cast<uint32_t>(time(nullptr));
    nextActionMs = millis() + REFRESH_INTERVAL_MS;

    Serial.print("[AircraftService] Tracking ");
    Serial.print(aircraftCount);
    Serial.print(" of ");
    Serial.print(reportedAircraft);
    Serial.println(" reported aircraft within 25 nm");
}

bool AircraftService::fetchFromSource(
    const String& url,
    const char* sourceName,
    uint8_t& reportedAircraft)
{
    reportedAircraft = 0;

    WiFiClientSecure secureClient;
    secureClient.setInsecure();

    HTTPClient http;
    http.setConnectTimeout(REQUEST_TIMEOUT_MS);
    http.setTimeout(REQUEST_TIMEOUT_MS);
    http.setUserAgent("MissionControl-ESP32/1.0");

    if (!http.begin(secureClient, url))
    {
        lastError = String(sourceName) + ": unable to start request";
        return false;
    }

    const int responseCode = http.GET();
    if (responseCode < 200 || responseCode >= 300)
    {
        lastError = responseCode < 0
            ? String(sourceName) + ": " + HTTPClient::errorToString(responseCode)
            : String(sourceName) + " HTTP " + responseCode;
        http.end();
        return false;
    }

    // Buffering the bounded regional response lets HTTPClient completely
    // decode chunked transfer framing before ArduinoJson examines the body.
    const String response = http.getString();
    http.end();
    if (response.isEmpty())
    {
        lastError = String(sourceName) + ": empty response";
        return false;
    }

    JsonDocument filter;
    JsonObject itemFilter = filter["ac"].add<JsonObject>();
    itemFilter["hex"] = true;
    itemFilter["flight"] = true;
    itemFilter["r"] = true;
    itemFilter["t"] = true;
    itemFilter["desc"] = true;
    itemFilter["category"] = true;
    itemFilter["dbFlags"] = true;
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
        response,
        DeserializationOption::Filter(filter));

    if (error)
    {
        lastError = String(sourceName) + " data: " + error.c_str();
        return false;
    }

    const JsonArrayConst sources = document["ac"].as<JsonArrayConst>();
    reportedAircraft = static_cast<uint8_t>(
        min(static_cast<size_t>(255), sources.isNull() ? 0 : sources.size()));
    aircraftCount = 0;
    for (JsonObjectConst source : sources)
    {
        if (aircraftCount >= MAX_AIRCRAFT ||
            source["lat"].isNull() || source["lon"].isNull())
        {
            continue;
        }

        // ArduinoJson may retain decimal coordinates as either float or
        // double depending on its build configuration. Convert any numeric
        // representation instead of rejecting everything that is not stored
        // specifically as a double.
        const double aircraftLatitude = source["lat"].as<double>();
        const double aircraftLongitude = source["lon"].as<double>();
        if (isnan(aircraftLatitude) || isinf(aircraftLatitude) ||
            isnan(aircraftLongitude) || isinf(aircraftLongitude))
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
        target.databaseFlags = source["dbFlags"] | 0;
        target.emergency = cleanString(source["emergency"] | "");
        target.squawk = cleanString(source["squawk"] | "");
        target.latitude = aircraftLatitude;
        target.longitude = aircraftLongitude;
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

    Serial.print("[AircraftService] ");
    Serial.print(sourceName);
    Serial.print(" accepted ");
    Serial.print(aircraftCount);
    Serial.print(" of ");
    Serial.print(reportedAircraft);
    Serial.println(" records");
    lastError = "";
    return true;
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
