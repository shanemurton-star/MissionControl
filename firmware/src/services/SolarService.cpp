#include "SolarService.h"

#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <time.h>

namespace
{
    const char* BASE = "https://services.swpc.noaa.gov";
}

void SolarService::begin() { nextUpdateMs = 0; }

void SolarService::update()
{
    if (updating || !timeReached(nextUpdateMs) || WiFi.status() != WL_CONNECTED) return;
    updating = true;
    lastError = "";
    if (fetchAll())
    {
        valid = true;
        data.updatedAt = static_cast<uint32_t>(time(nullptr));
        nextUpdateMs = millis() + REFRESH_MS;
        Serial.println("[SolarService] NOAA space weather updated");
    }
    else
    {
        nextUpdateMs = millis() + RETRY_MS;
    }
    updating = false;
}

bool SolarService::fetchAll()
{
    String payload;
    JsonDocument document;

    if (!fetchJson("/products/summary/10cm-flux.json", payload) ||
        deserializeJson(document, payload)) return false;
    data.solarFlux = document[0]["flux"] | 0.0f;

    document.clear(); payload = "";
    if (!fetchJson("/products/noaa-planetary-k-index.json", payload) ||
        deserializeJson(document, payload)) return false;
    JsonArray indices = document.as<JsonArray>();
    if (indices.size() == 0) { lastError = "NOAA K-index data was empty"; return false; }
    data.kpIndex = indices[indices.size() - 1]["Kp"] | 0.0f;
    data.aIndex = indices[indices.size() - 1]["a_running"] | 0;

    document.clear(); payload = "";
    if (!fetchJson("/json/goes/primary/xray-flares-latest.json", payload) ||
        deserializeJson(document, payload)) return false;
    data.xrayClass = String(document[0]["current_class"] | "--");
    data.flarePeakClass = String(document[0]["max_class"] | "--");

    document.clear(); payload = "";
    if (!fetchJson("/products/noaa-scales.json", payload) ||
        deserializeJson(document, payload)) return false;
    data.radioBlackoutScale = String(document["0"]["R"]["Scale"] | "0").toInt();
    data.solarRadiationScale = String(document["0"]["S"]["Scale"] | "0").toInt();
    data.geomagneticStormScale = String(document["0"]["G"]["Scale"] | "0").toInt();

    document.clear(); payload = "";
    if (!fetchJson("/products/summary/solar-wind-speed.json", payload) ||
        deserializeJson(document, payload)) return false;
    data.solarWindSpeed = document[0]["proton_speed"] | 0.0f;

    document.clear(); payload = "";
    if (!fetchJson("/products/summary/solar-wind-mag-field.json", payload) ||
        deserializeJson(document, payload)) return false;
    data.magneticField = document[0]["bt"] | 0.0f;
    return true;
}

bool SolarService::fetchJson(const char* path, String& payload)
{
    HTTPClient http;
    http.setConnectTimeout(REQUEST_TIMEOUT_MS);
    http.setTimeout(REQUEST_TIMEOUT_MS);
    http.setUserAgent("MissionControl-ESP32/1.0");
    if (!http.begin(String(BASE) + path))
    {
        lastError = "Unable to start NOAA request";
        return false;
    }
    const int response = http.GET();
    if (response < 200 || response >= 300)
    {
        lastError = String("NOAA SWPC HTTP ") + response;
        http.end();
        return false;
    }
    payload = http.getString();
    http.end();
    if (payload.isEmpty()) { lastError = "NOAA SWPC returned no data"; return false; }
    return true;
}

bool SolarService::isValid() const { return valid; }
bool SolarService::isUpdating() const { return updating; }
const SolarData& SolarService::getData() const { return data; }
const String& SolarService::getLastError() const { return lastError; }

const char* SolarService::getPropagationLabel() const
{
    if (!valid) return "UNKNOWN";
    if (data.radioBlackoutScale >= 2 || data.kpIndex >= 6.0f) return "POOR";
    if (data.radioBlackoutScale >= 1 || data.kpIndex >= 4.0f) return "VARIABLE";
    if (data.solarFlux >= 150.0f && data.kpIndex < 3.0f) return "EXCELLENT";
    if (data.solarFlux >= 100.0f && data.kpIndex < 4.0f) return "GOOD";
    return "FAIR";
}

bool SolarService::timeReached(unsigned long target)
{
    return static_cast<long>(millis() - target) >= 0;
}
