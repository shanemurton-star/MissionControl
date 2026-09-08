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
        {"SO-50", 27607},
        {"AO-91", 43017},
        {"RS-44", 44909},
        {"JO-97", 43803},
        {"FO-29", 24278},
        {"AO-7", 7530},
        {"TEVEL2-3", 63218},
        {"TEVEL2-7", 63238},
        {"NOAA 15", 25338},
        {"NOAA 18", 28654},
        {"NOAA 19", 33591}
    };

    void trimLine(String& line)
    {
        line.trim();
        if (line.endsWith("\r")) line.remove(line.length() - 1);
    }

    bool parseThreeLineTle(
        String payload, String& name, String& line1, String& line2)
    {
        payload.replace("\r", "");
        const int firstBreak = payload.indexOf('\n');
        if (firstBreak < 0) return false;
        const int secondBreak = payload.indexOf('\n', firstBreak + 1);
        if (secondBreak < 0) return false;
        int thirdBreak = payload.indexOf('\n', secondBreak + 1);
        if (thirdBreak < 0) thirdBreak = payload.length();

        name = payload.substring(0, firstBreak);
        line1 = payload.substring(firstBreak + 1, secondBreak);
        line2 = payload.substring(secondBreak + 1, thirdBreak);
        trimLine(name);
        trimLine(line1);
        trimLine(line2);
        return line1.startsWith("1 ") && line2.startsWith("2 ");
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
    cacheReady = cachePreferences.begin("satcache", false);
    if (cacheReady) loadCachedTles();
    else Serial.println("[SatelliteService] Unable to open orbit cache");
    // Weather and aircraft make several HTTPS connections during startup.
    // Let those sockets clear before beginning the paced TLE refresh.
    nextTleRefreshMs = millis() + INITIAL_TLE_REFRESH_DELAY_MS;
}

void SatelliteService::update()
{
    // SGP4 pass prediction needs a valid UTC epoch. Avoid calculating
    // against the ESP32's 1970 startup clock while NTP is still syncing.
    if (time(nullptr) < 1704067200)
    {
        return;
    }

    if (!cacheEvaluated)
    {
        cacheEvaluated = true;
        if (activateCachedTles()) return;
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
    const bool fetched = fetchTle(fetchIndex);
    if (fetched)
    {
        tleLoaded[fetchIndex] = true;
        tleFromCache[fetchIndex] = false;
        tleUpdatedAt[fetchIndex] = static_cast<uint32_t>(time(nullptr));
    }
    else
    {
        Serial.print("[SatelliteService] Skipping unavailable TLE for ");
        Serial.println(DEFINITIONS[fetchIndex].name);
        // Both providers failed before returning usable data. Continuing the
        // batch would turn one outage or temporary block into 24 connection
        // attempts. Retain every cached TLE and resume at this satellite
        // after a long backoff instead of refetching earlier successes.
        updating = false;
        nextTleRefreshMs = millis() + SOURCE_FAILURE_BACKOFF_MS;
        Serial.println(
            "[SatelliteService] TLE sources unavailable; cached orbit data "
            "retained, retrying in 2 hours");
        return;
    }

    ++fetchIndex;
    if (fetchIndex < SATELLITE_COUNT)
    {
        updating = false;
        nextTleRefreshMs = millis() + TLE_FETCH_GAP_MS;
        return;
    }

    fetchIndex = 0;
    uint8_t loadedCount = 0;
    for (bool loaded : tleLoaded)
        if (loaded) ++loadedCount;
    if (loadedCount > 0)
    {
        calculatePasses();
        valid = true;
        lastError = "";
        usingCachedData = false;
        for (uint8_t index = 0; index < SATELLITE_COUNT; ++index)
            if (tleLoaded[index] && tleFromCache[index])
                usingCachedData = true;
    }
    else
    {
        valid = false;
        if (lastError.isEmpty()) lastError = "No satellite orbit data available";
    }
    updating = false;
    nextTleRefreshMs = millis() + TLE_REFRESH_MS;
    if (valid) updatePositions();
    Serial.print("[SatelliteService] Orbit data ready for ");
    Serial.print(loadedCount);
    Serial.print("/");
    Serial.print(SATELLITE_COUNT);
    Serial.println(" satellites");
}

bool SatelliteService::fetchTle(uint8_t index)
{
    String name;
    String line1;
    String line2;
    bool tleReceived = false;

    // Fetch one compact JSON TLE record by NORAD catalog ID. Once CelesTrak
    // has successfully served as a fallback, prefer it for the rest of the
    // batch rather than repeatedly waiting on an unavailable SatNOGS host.
    if (!preferCelestrak)
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
        const String url =
            String("https://celestrak.org/NORAD/elements/gp.php?CATNR=") +
            DEFINITIONS[index].catalogNumber + "&FORMAT=TLE";
        WiFiClientSecure client;
        client.setInsecure();
        client.setHandshakeTimeout(5);
        client.setTimeout(REQUEST_TIMEOUT_MS);
        HTTPClient http;
        http.setConnectTimeout(REQUEST_TIMEOUT_MS);
        http.setTimeout(REQUEST_TIMEOUT_MS);
        http.setUserAgent("MissionControl-ESP32/1.0");
        Serial.print("[SatelliteService] Trying CelesTrak fallback for ");
        Serial.println(DEFINITIONS[index].name);
        if (http.begin(client, url))
        {
            const int responseCode = http.GET();
            Serial.print("[SatelliteService] CelesTrak response: ");
            Serial.println(responseCode);
            if (responseCode >= 200 && responseCode < 300)
            {
                tleReceived = parseThreeLineTle(
                    http.getString(), name, line1, line2);
                if (!tleReceived)
                    lastError = "Invalid CelesTrak TLE response";
                else
                    preferCelestrak = true;
            }
            else
            {
                lastError = String("CelesTrak TLE HTTP ") + responseCode;
            }
            http.end();
        }
        else
        {
            lastError = "Unable to start CelesTrak request";
        }

        if (!tleReceived)
        {
            preferCelestrak = false;
            if (lastError.isEmpty())
                lastError = "Unable to connect to TLE sources";
            return false;
        }
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

    if (!initializeTle(index, name, line1, line2))
    {
        lastError = String("Unable to initialize ") + DEFINITIONS[index].name;
        return false;
    }
    saveCachedTle(index, name, line1, line2);
    return true;
}

bool SatelliteService::initializeTle(
    uint8_t index, const String& name,
    const String& line1, const String& line2)
{
    char nameBuffer[25];
    char line1Buffer[130];
    char line2Buffer[130];
    name.substring(0, 24).toCharArray(nameBuffer, sizeof(nameBuffer));
    line1.toCharArray(line1Buffer, sizeof(line1Buffer));
    line2.toCharArray(line2Buffer, sizeof(line2Buffer));
    return predictors[index].init(nameBuffer, line1Buffer, line2Buffer);
}

void SatelliteService::loadCachedTles()
{
    uint8_t loadedCount = 0;
    for (uint8_t index = 0; index < SATELLITE_COUNT; ++index)
    {
        char nameKey[8];
        char line1Key[8];
        char line2Key[8];
        char timeKey[8];
        snprintf(nameKey, sizeof(nameKey), "n%u", index);
        snprintf(line1Key, sizeof(line1Key), "a%u", index);
        snprintf(line2Key, sizeof(line2Key), "b%u", index);
        snprintf(timeKey, sizeof(timeKey), "t%u", index);
        const String line1 = cachePreferences.getString(line1Key, "");
        const String line2 = cachePreferences.getString(line2Key, "");
        if (!line1.startsWith("1 ") || !line2.startsWith("2 ")) continue;
        const String name = cachePreferences.getString(
            nameKey, DEFINITIONS[index].name);
        if (!initializeTle(index, name, line1, line2)) continue;
        tleLoaded[index] = true;
        tleFromCache[index] = true;
        tleUpdatedAt[index] = cachePreferences.getUInt(timeKey, 0);
        ++loadedCount;
    }
    Serial.print("[SatelliteService] Loaded ");
    Serial.print(loadedCount);
    Serial.println(" cached TLEs");
}

void SatelliteService::saveCachedTle(
    uint8_t index, const String& name,
    const String& line1, const String& line2)
{
    if (!cacheReady) return;
    char nameKey[8];
    char line1Key[8];
    char line2Key[8];
    char timeKey[8];
    snprintf(nameKey, sizeof(nameKey), "n%u", index);
    snprintf(line1Key, sizeof(line1Key), "a%u", index);
    snprintf(line2Key, sizeof(line2Key), "b%u", index);
    snprintf(timeKey, sizeof(timeKey), "t%u", index);
    cachePreferences.putString(nameKey, name);
    cachePreferences.putString(line1Key, line1);
    cachePreferences.putString(line2Key, line2);
    cachePreferences.putUInt(timeKey, static_cast<uint32_t>(time(nullptr)));
}

bool SatelliteService::activateCachedTles()
{
    const uint32_t now = static_cast<uint32_t>(time(nullptr));
    uint8_t usableCount = 0;
    for (uint8_t index = 0; index < SATELLITE_COUNT; ++index)
    {
        if (!tleLoaded[index] || !tleFromCache[index]) continue;
        const uint32_t updatedAt = tleUpdatedAt[index];
        if (updatedAt == 0 || now < updatedAt ||
            now - updatedAt > MAX_CACHE_AGE_SECONDS)
        {
            tleLoaded[index] = false;
            tleFromCache[index] = false;
            continue;
        }
        ++usableCount;
    }
    if (usableCount == 0) return false;

    calculatePasses();
    valid = true;
    usingCachedData = true;
    lastError = "";
    updatePositions();
    Serial.print("[SatelliteService] Using ");
    Serial.print(usableCount);
    Serial.println(" cached TLEs while refreshing in background");
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
        if (!tleLoaded[i])
        {
            satellites[i].valid = false;
            satellites[i].visible = false;
            continue;
        }
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
        if (!tleLoaded[i])
        {
            satellites[i].visible = false;
            continue;
        }
        predictors[i].findsat(static_cast<unsigned long>(now));
        satellites[i].currentAzimuth = predictors[i].satAz;
        satellites[i].currentElevation = predictors[i].satEl;
        satellites[i].rangeKm = predictors[i].satDist;
        satellites[i].visible = predictors[i].satEl >= 0.0;
        predictors[i].findsat(static_cast<unsigned long>(now + 10));
        satellites[i].futureAzimuth = predictors[i].satAz;
        satellites[i].futureElevation = predictors[i].satEl;
        if (satellites[i].valid && now >= satellites[i].losTime) needsNewPasses = true;
    }
    if (needsNewPasses) calculatePasses();
}

bool SatelliteService::isValid() const { return valid; }
bool SatelliteService::isUpdating() const { return updating; }
const SatelliteData& SatelliteService::getSatellite(uint8_t index) const { return satellites[index]; }
const String& SatelliteService::getLastError() const { return lastError; }
bool SatelliteService::isUsingCachedData() const { return usingCachedData; }

void SatelliteService::requestRadioData(uint8_t index)
{
    if (index >= SATELLITE_COUNT || satellites[index].radioReady) return;
    pendingRadioIndex = static_cast<int8_t>(index);
}

const SatelliteData* SatelliteService::getNextPass() const
{
    const uint32_t now = static_cast<uint32_t>(time(nullptr));
    const SatelliteData* next = nullptr;
    for (const SatelliteData& satellite : satellites)
    {
        // An active pass is represented separately by the panel's IN PASS
        // state. Never present its already elapsed AOS as an upcoming pass.
        if (!satellite.valid || satellite.aosTime <= now) continue;
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
