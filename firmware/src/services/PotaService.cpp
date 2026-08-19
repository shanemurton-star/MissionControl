#include "PotaService.h"

#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <math.h>

namespace
{
    constexpr double EARTH_RADIUS_MILES = 3958.7613;
    const char* POTA_SPOTS_URL = "https://api.pota.app/spot/activator";
}

void PotaService::begin(const AppSettings& settings)
{
    homeLatitude = settings.latitude;
    homeLongitude = settings.longitude;
    nextUpdateMs = 0;
}

void PotaService::update()
{
    if (updating || !timeReached(nextUpdateMs) || WiFi.status() != WL_CONNECTED) return;
    fetchSpots();
}

void PotaService::fetchSpots()
{
    updating = true;
    lastError = "";

    HTTPClient http;
    http.setConnectTimeout(REQUEST_TIMEOUT_MS);
    http.setTimeout(REQUEST_TIMEOUT_MS);
    http.setUserAgent("MissionControl-ESP32/1.0");
    if (!http.begin(POTA_SPOTS_URL))
    {
        lastError = "Unable to start POTA request";
        updating = false;
        nextUpdateMs = millis() + RETRY_MS;
        return;
    }

    const int response = http.GET();
    if (response < 200 || response >= 300)
    {
        lastError = String("POTA API HTTP ") + response;
        http.end();
        updating = false;
        nextUpdateMs = millis() + RETRY_MS;
        return;
    }

    JsonDocument filter;
    JsonObject item = filter[0].to<JsonObject>();
    item["activator"] = true;
    item["frequency"] = true;
    item["mode"] = true;
    item["reference"] = true;
    item["latitude"] = true;
    item["longitude"] = true;
    item["comments"] = true;
    item["name"] = true;
    item["locationDesc"] = true;

    JsonDocument document;
    const DeserializationError error = deserializeJson(
        document,
        http.getStream(),
        DeserializationOption::Filter(filter));
    http.end();
    if (error)
    {
        lastError = String("POTA data error: ") + error.c_str();
        updating = false;
        nextUpdateMs = millis() + RETRY_MS;
        return;
    }

    spotCount = 0;
    for (JsonObject source : document.as<JsonArray>())
    {
        String comments = source["comments"] | "";
        comments.toUpperCase();
        if (comments.indexOf("QRT") >= 0) continue;

        const double latitude = source["latitude"] | 0.0;
        const double longitude = source["longitude"] | 0.0;
        const char* activator = source["activator"] | "";
        const char* reference = source["reference"] | "";
        if (latitude == 0.0 || longitude == 0.0 || activator[0] == '\0' || reference[0] == '\0') continue;

        PotaSpotData spot;
        spot.activator = activator;
        spot.frequency = String(source["frequency"] | "");
        spot.mode = String(source["mode"] | "");
        spot.reference = reference;
        spot.name = String(source["name"] | "");
        spot.location = String(source["locationDesc"] | "");
        spot.distanceMiles = distanceMiles(latitude, longitude);
        if (spot.distanceMiles > ACTIVE_RADIUS_MILES) continue;
        insertSpot(spot);
    }

    valid = true;
    updating = false;
    nextUpdateMs = millis() + REFRESH_MS;
}

void PotaService::insertSpot(const PotaSpotData& spot)
{
    // Count parks, not duplicate spots for the same park reference.
    for (uint8_t index = 0; index < spotCount; ++index)
        if (spots[index].reference == spot.reference) return;

    uint8_t position = 0;
    while (position < spotCount && spots[position].distanceMiles <= spot.distanceMiles) ++position;
    if (position >= MAX_SPOTS) return;

    const uint8_t last = spotCount < MAX_SPOTS ? spotCount : MAX_SPOTS - 1;
    for (uint8_t index = last; index > position; --index) spots[index] = spots[index - 1];
    spots[position] = spot;
    if (spotCount < MAX_SPOTS) ++spotCount;
}

float PotaService::distanceMiles(double latitude, double longitude) const
{
    const double lat1 = homeLatitude * DEG_TO_RAD;
    const double lat2 = latitude * DEG_TO_RAD;
    const double deltaLat = (latitude - homeLatitude) * DEG_TO_RAD;
    const double deltaLon = (longitude - homeLongitude) * DEG_TO_RAD;
    const double a = sin(deltaLat / 2.0) * sin(deltaLat / 2.0) +
        cos(lat1) * cos(lat2) * sin(deltaLon / 2.0) * sin(deltaLon / 2.0);
    return static_cast<float>(EARTH_RADIUS_MILES * 2.0 * atan2(sqrt(a), sqrt(1.0 - a)));
}

bool PotaService::isValid() const { return valid; }
bool PotaService::isUpdating() const { return updating; }
uint8_t PotaService::getSpotCount() const { return spotCount; }
const PotaSpotData& PotaService::getSpot(uint8_t index) const { return spots[index]; }
const String& PotaService::getLastError() const { return lastError; }
bool PotaService::timeReached(uint32_t target)
{
    return static_cast<int32_t>(millis() - target) >= 0;
}
