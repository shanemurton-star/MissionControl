#include "SatelliteService.h"

#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <math.h>
#include <time.h>

namespace
{
    struct SatelliteDefinition
    {
        const char* name;
        uint32_t catalogNumber;
    };

    constexpr SatelliteDefinition DEFINITIONS[SatelliteService::SATELLITE_COUNT] = {
        {"ISS (ZARYA)", 25544},
        {"NOAA 15", 25338},
        {"NOAA 18", 28654},
        {"NOAA 19", 33591},
        {"SO-50", 27607}
    };

    void trimLine(String& line)
    {
        line.trim();
        if (line.endsWith("\r")) line.remove(line.length() - 1);
    }
}

void SatelliteService::begin(const AppSettings& settings)
{
    latitude = settings.latitude;
    longitude = settings.longitude;
    for (uint8_t i = 0; i < SATELLITE_COUNT; ++i)
    {
        satellites[i].name = DEFINITIONS[i].name;
        satellites[i].catalogNumber = DEFINITIONS[i].catalogNumber;
        predictors[i].site(latitude, longitude, 0.0);
    }
}

void SatelliteService::update()
{
    // SGP4 pass prediction needs a valid UTC epoch. Avoid calculating
    // against the ESP32's 1970 startup clock while NTP is still syncing.
    if (time(nullptr) < 1704067200)
    {
        return;
    }

    if (WiFi.status() == WL_CONNECTED && !updating && timeReached(nextTleRefreshMs))
    {
        fetchNextTle();
    }

    if (valid && timeReached(nextPositionUpdateMs))
    {
        updatePositions();
        nextPositionUpdateMs = millis() + POSITION_UPDATE_MS;
    }

    if (pendingRadioIndex >= 0 && WiFi.status() == WL_CONNECTED && !updating)
    {
        const uint8_t index = static_cast<uint8_t>(pendingRadioIndex);
        pendingRadioIndex = -1;
        updating = true;
        fetchRadioData(index);
        updating = false;
    }
}

void SatelliteService::fetchNextTle()
{
    updating = true;
    if (!fetchTle(fetchIndex))
    {
        updating = false;
        nextTleRefreshMs = millis() + RETRY_MS;
        return;
    }

    ++fetchIndex;
    if (fetchIndex < SATELLITE_COUNT)
    {
        updating = false;
        return;
    }

    fetchIndex = 0;
    calculatePasses();
    valid = true;
    updating = false;
    lastError = "";
    nextTleRefreshMs = millis() + TLE_REFRESH_MS;
    updatePositions();
    Serial.println("[SatelliteService] TLEs and passes updated");
}

bool SatelliteService::fetchTle(uint8_t index)
{
    String name;
    String line1;
    String line2;
    bool tleReceived = false;

    // SatNOGS is reachable from the station network while CelesTrak currently
    // times out from both the ESP32 and a desktop on the same connection.
    // Fetch one compact JSON TLE record by NORAD catalog ID.
    {
        const String url = String("https://db.satnogs.org/api/tle/?norad_cat_id=") +
            DEFINITIONS[index].catalogNumber + "&format=json";
        WiFiClientSecure client;
        client.setInsecure();
        client.setHandshakeTimeout(5);
        client.setTimeout(REQUEST_TIMEOUT_MS);
        HTTPClient http;
        http.setConnectTimeout(REQUEST_TIMEOUT_MS);
        http.setTimeout(REQUEST_TIMEOUT_MS);
        http.setUserAgent("MissionControl-ESP32/1.0");
        Serial.print("[SatelliteService] Fetching TLE ");
        Serial.print(index + 1);
        Serial.print("/");
        Serial.print(SATELLITE_COUNT);
        Serial.print(": ");
        Serial.println(DEFINITIONS[index].name);
        if (http.begin(client, url))
        {
            const int responseCode = http.GET();
            Serial.print("[SatelliteService] SatNOGS response: ");
            Serial.println(responseCode);
            if (responseCode >= 200 && responseCode < 300)
            {
                JsonDocument document;
                const DeserializationError error =
                    deserializeJson(document, http.getStream());
                JsonArrayConst results = document.as<JsonArrayConst>();
                if (!error && !results.isNull() && results.size() > 0)
                {
                    name = String(results[0]["tle0"] | DEFINITIONS[index].name);
                    line1 = String(results[0]["tle1"] | "");
                    line2 = String(results[0]["tle2"] | "");
                    tleReceived = true;
                }
                else lastError = "Invalid SatNOGS TLE response";
            }
            else
            {
                lastError = String("SatNOGS TLE HTTP ") + responseCode;
            }
            http.end();
        }
        else
        {
            lastError = "Unable to start SatNOGS request";
        }
    }

    if (!tleReceived)
    {
        if (lastError.isEmpty()) lastError = "Unable to connect to TLE sources";
        return false;
    }

    if (name.startsWith("0 ")) name.remove(0, 2);
    trimLine(name);
    trimLine(line1);
    trimLine(line2);

    if (!line1.startsWith("1 ") || !line2.startsWith("2 "))
    {
        lastError = String("Invalid TLE for ") + DEFINITIONS[index].name;
        return false;
    }

    char nameBuffer[25];
    char line1Buffer[130];
    char line2Buffer[130];
    name.substring(0, 24).toCharArray(nameBuffer, sizeof(nameBuffer));
    line1.toCharArray(line1Buffer, sizeof(line1Buffer));
    line2.toCharArray(line2Buffer, sizeof(line2Buffer));
    if (!predictors[index].init(nameBuffer, line1Buffer, line2Buffer))
    {
        lastError = String("Unable to initialize ") + DEFINITIONS[index].name;
        return false;
    }
    return true;
}

bool SatelliteService::fetchRadioData(uint8_t index)
{
    SatelliteData& satellite = satellites[index];
    satellite.radioChannelCount = 0;
    satellite.radioError = "";

    const String url = String("https://db.satnogs.org/api/transmitters/?satellite__norad_cat_id=") +
        satellite.catalogNumber + "&format=json";
    WiFiClientSecure client;
    client.setInsecure();
    client.setHandshakeTimeout(5);
    client.setTimeout(REQUEST_TIMEOUT_MS);
    HTTPClient http;
    http.setConnectTimeout(REQUEST_TIMEOUT_MS);
    http.setTimeout(REQUEST_TIMEOUT_MS);
    http.setUserAgent("MissionControl-ESP32/1.0");
    Serial.print("[SatelliteService] Fetching radio data for ");
    Serial.println(satellite.name);
    if (!http.begin(client, url))
    {
        satellite.radioError = "Unable to start SatNOGS request";
        satellite.radioReady = true;
        return false;
    }
    const int responseCode = http.GET();
    Serial.print("[SatelliteService] SatNOGS radio response: ");
    Serial.println(responseCode);
    if (responseCode < 200 || responseCode >= 300)
    {
        satellite.radioError = String("SatNOGS HTTP ") + responseCode;
        satellite.radioReady = true;
        http.end();
        return false;
    }

    JsonDocument filter;
    JsonObject selected = filter[0].to<JsonObject>();
    selected["description"] = true;
    selected["alive"] = true;
    selected["status"] = true;
    selected["service"] = true;
    selected["mode"] = true;
    selected["uplink_low"] = true;
    selected["downlink_low"] = true;
    selected["baud"] = true;

    JsonDocument document;
    const DeserializationError error = deserializeJson(
        document, http.getStream(), DeserializationOption::Filter(filter));
    http.end();
    if (error)
    {
        satellite.radioError = String("SatNOGS data error: ") + error.c_str();
        satellite.radioReady = true;
        return false;
    }

    // Amateur entries are the most useful on this dashboard. Fill remaining
    // space with other active transmitters for weather and utility satellites.
    for (uint8_t priority = 0; priority < 2; ++priority)
    {
        for (JsonObjectConst source : document.as<JsonArrayConst>())
        {
            if (satellite.radioChannelCount >= SatelliteData::MAX_RADIO_CHANNELS) break;
            const bool alive = source["alive"] | false;
            const String status = String(source["status"] | "");
            const String service = String(source["service"] | "");
            const bool amateur = service.equalsIgnoreCase("Amateur");
            if (!alive || !status.equalsIgnoreCase("active") || amateur != (priority == 0)) continue;

            const uint64_t uplink = source["uplink_low"].isNull()
                ? 0 : source["uplink_low"].as<uint64_t>();
            const uint64_t downlink = source["downlink_low"].isNull()
                ? 0 : source["downlink_low"].as<uint64_t>();
            if (uplink == 0 && downlink == 0) continue;

            SatelliteRadioChannel& channel =
                satellite.radioChannels[satellite.radioChannelCount++];
            channel.description = String(source["description"] | "Radio channel");
            channel.mode = String(source["mode"] | "--");
            channel.service = service;
            channel.uplinkHz = uplink;
            channel.downlinkHz = downlink;
            channel.baud = source["baud"].isNull()
                ? 0 : static_cast<uint32_t>(source["baud"].as<float>() + 0.5f);
        }
    }

    satellite.radioReady = true;
    Serial.print("[SatelliteService] SatNOGS radio channels for ");
    Serial.print(satellite.name);
    Serial.print(": ");
    Serial.println(satellite.radioChannelCount);
    return true;
}

void SatelliteService::calculatePasses()
{
    const uint32_t now = static_cast<uint32_t>(time(nullptr));
    for (uint8_t i = 0; i < SATELLITE_COUNT; ++i)
    {
        passinfo pass;
        predictors[i].initpredpoint(static_cast<unsigned long>(now), 0.0);
        satellites[i].valid = predictors[i].nextpass(&pass, 30, false, 5.0);
        if (!satellites[i].valid) continue;
        satellites[i].aosTime = julianToUnix(pass.jdstart);
        satellites[i].maxTime = julianToUnix(pass.jdmax);
        satellites[i].losTime = julianToUnix(pass.jdstop);
        satellites[i].maxElevation = pass.maxelevation;
        satellites[i].aosAzimuth = pass.azstart;
        satellites[i].losAzimuth = pass.azstop;
    }
}

void SatelliteService::updatePositions()
{
    const uint32_t now = static_cast<uint32_t>(time(nullptr));
    bool needsNewPasses = false;
    for (uint8_t i = 0; i < SATELLITE_COUNT; ++i)
    {
        predictors[i].findsat(static_cast<unsigned long>(now));
        satellites[i].currentAzimuth = predictors[i].satAz;
        satellites[i].currentElevation = predictors[i].satEl;
        satellites[i].rangeKm = predictors[i].satDist;
        satellites[i].visible = predictors[i].satEl >= 0.0;
        if (satellites[i].valid && now > satellites[i].losTime) needsNewPasses = true;
    }
    if (needsNewPasses) calculatePasses();
}

bool SatelliteService::isValid() const { return valid; }
bool SatelliteService::isUpdating() const { return updating; }
const SatelliteData& SatelliteService::getSatellite(uint8_t index) const { return satellites[index]; }
const String& SatelliteService::getLastError() const { return lastError; }

void SatelliteService::requestRadioData(uint8_t index)
{
    if (index >= SATELLITE_COUNT || satellites[index].radioReady) return;
    pendingRadioIndex = static_cast<int8_t>(index);
}

const SatelliteData* SatelliteService::getNextPass() const
{
    const SatelliteData* next = nullptr;
    for (const SatelliteData& satellite : satellites)
    {
        if (!satellite.valid) continue;
        if (next == nullptr || satellite.aosTime < next->aosTime) next = &satellite;
    }
    return next;
}

uint32_t SatelliteService::julianToUnix(double julianDay)
{
    return static_cast<uint32_t>((julianDay - 2440587.5) * 86400.0 + 0.5);
}

bool SatelliteService::timeReached(unsigned long targetTime)
{
    return static_cast<long>(millis() - targetTime) >= 0;
}
