#include "LiveSpotsService.h"
#include <HTTPClient.h>
#include <WiFi.h>
#include <math.h>

namespace
{
    constexpr const char* BAND_NAMES[LiveSpotsService::BAND_COUNT] =
        {"80m", "60m", "40m", "30m", "20m", "17m", "15m", "12m", "10m"};
    constexpr double EARTH_RADIUS_KM = 6371.0;
}

void LiveSpotsService::begin(const AppSettings& settings)
{
    gridSquare = settings.gridSquare;
    gridSquare.trim();
    if (gridSquare.length() >= 2)
    {
        String field = gridSquare.substring(0, 2);
        field.toUpperCase();
        gridSquare = field + gridSquare.substring(2);
    }
    if (gridSquare.length() == 6)
    {
        String subsquare = gridSquare.substring(4, 6);
        subsquare.toLowerCase();
        gridSquare = gridSquare.substring(0, 4) + subsquare;
    }
    queryGrid = gridSquare.substring(0, min(static_cast<unsigned int>(4), gridSquare.length()));
    if (!gridToCoordinates(gridSquare, homeLatitude, homeLongitude))
    {
        homeLatitude = settings.latitude;
        homeLongitude = settings.longitude;
    }
    for (uint8_t i = 0; i < BAND_COUNT; ++i) bands[i].name = BAND_NAMES[i];
    recentSpotCount = 0;
    totalSpotCount = 0;
    nextUpdateMs = 0;
    valid = false;
    updating = false;
    lastError = "";
}

void LiveSpotsService::update()
{
    if (updating || !timeReached(nextUpdateMs) || WiFi.status() != WL_CONNECTED) return;
    updating = true;
    if (fetchReports())
    {
        valid = true;
        nextUpdateMs = millis() + REFRESH_MS;
    }
    else nextUpdateMs = millis() + RETRY_MS;
    updating = false;
}

bool LiveSpotsService::fetchReports()
{
    const String url = String("https://retrieve.pskreporter.info/query?receiverCallsign=") +
        queryGrid + "&modify=grid&flowStartSeconds=-3600&rptlimit=100&rronly=1";
    HTTPClient http;
    http.setConnectTimeout(REQUEST_TIMEOUT_MS);
    http.setTimeout(REQUEST_TIMEOUT_MS);
    http.setUserAgent("MissionControl-ESP32/1.0");
    if (!http.begin(url)) { lastError = "Unable to start PSK query"; return false; }
    const int response = http.GET();
    if (response < 200 || response >= 300)
    {
        lastError = String("PSK Reporter HTTP ") + response;
        http.end();
        return false;
    }
    const int size = http.getSize();
    if (size > 160 * 1024) { lastError = "PSK response was too large"; http.end(); return false; }
    String xml = http.getString();
    http.end();
    if (xml.indexOf("<receptionReports") < 0) { lastError = "Invalid PSK Reporter data"; return false; }
    parseReports(xml);
    lastError = "";
    Serial.print("[LiveSpotsService] Parsed ");
    Serial.print(totalSpotCount);
    Serial.print(" ");
    Serial.print(gridSquare);
    Serial.println(" reports");
    return true;
}

void LiveSpotsService::parseReports(const String& xml)
{
    for (uint8_t i = 0; i < BAND_COUNT; ++i)
    {
        bands[i].count = 0;
        bands[i].farthestKm = 0.0f;
        bands[i].farthestCall = "";
    }
    recentSpotCount = 0;
    totalSpotCount = 0;
    int position = 0;
    while ((position = xml.indexOf("<receptionReport ", position)) >= 0)
    {
        const int end = xml.indexOf("/>", position);
        if (end < 0) break;
        const String tag = xml.substring(position, end + 2);
        position = end + 2;
        const uint32_t frequency = static_cast<uint32_t>(attribute(tag, "frequency").toInt());
        const int8_t index = bandIndex(frequency);
        if (index < 0) continue;

        const String sender = attribute(tag, "senderCallsign");
        const String senderGrid = attribute(tag, "senderLocator");
        const float distance = distanceFromHome(senderGrid);
        ++bands[index].count;
        ++totalSpotCount;
        if (distance > bands[index].farthestKm)
        {
            bands[index].farthestKm = distance;
            bands[index].farthestCall = sender;
        }
        if (recentSpotCount < MAX_RECENT_SPOTS)
        {
            LiveSpot& spot = recentSpots[recentSpotCount++];
            spot.senderCallsign = sender;
            spot.senderLocator = senderGrid;
            spot.receiverCallsign = attribute(tag, "receiverCallsign");
            spot.mode = attribute(tag, "mode");
            spot.band = BAND_NAMES[index];
            spot.frequencyMhz = frequency / 1000000.0f;
            spot.distanceKm = distance;
            spot.snr = attribute(tag, "sNR").toInt();
            spot.timestamp = static_cast<uint32_t>(attribute(tag, "flowStartSeconds").toInt());
        }
    }
}

String LiveSpotsService::attribute(const String& tag, const char* name)
{
    const String prefix = String(name) + "=\"";
    const int start = tag.indexOf(prefix);
    if (start < 0) return "";
    const int valueStart = start + prefix.length();
    const int end = tag.indexOf('"', valueStart);
    return end < 0 ? String() : tag.substring(valueStart, end);
}

int8_t LiveSpotsService::bandIndex(uint32_t hz)
{
    if (hz >= 3500000 && hz <= 4000000) return 0;
    if (hz >= 5250000 && hz <= 5450000) return 1;
    if (hz >= 7000000 && hz <= 7300000) return 2;
    if (hz >= 10100000 && hz <= 10150000) return 3;
    if (hz >= 14000000 && hz <= 14350000) return 4;
    if (hz >= 18068000 && hz <= 18168000) return 5;
    if (hz >= 21000000 && hz <= 21450000) return 6;
    if (hz >= 24890000 && hz <= 24990000) return 7;
    if (hz >= 28000000 && hz <= 29700000) return 8;
    return -1;
}

bool LiveSpotsService::gridToCoordinates(const String& source, double& latitude, double& longitude)
{
    String grid = source; grid.trim(); grid.toUpperCase();
    if (grid.length() < 4 || grid[0] < 'A' || grid[0] > 'R' || grid[1] < 'A' || grid[1] > 'R') return false;
    longitude = -180.0 + (grid[0] - 'A') * 20.0 + (grid[2] - '0') * 2.0 + 1.0;
    latitude = -90.0 + (grid[1] - 'A') * 10.0 + (grid[3] - '0') + 0.5;
    if (grid.length() >= 6)
    {
        const char lonSub = tolower(grid[4]);
        const char latSub = tolower(grid[5]);
        if (lonSub >= 'a' && lonSub <= 'x' && latSub >= 'a' && latSub <= 'x')
        {
            longitude += (lonSub - 'a') * (2.0 / 24.0) - 1.0 + (1.0 / 24.0);
            latitude += (latSub - 'a') * (1.0 / 24.0) - 0.5 + (0.5 / 24.0);
        }
    }
    return true;
}

float LiveSpotsService::distanceFromHome(const String& grid) const
{
    double lat = 0.0, lon = 0.0;
    if (!gridToCoordinates(grid, lat, lon)) return 0.0f;
    const double lat1 = homeLatitude * DEG_TO_RAD, lat2 = lat * DEG_TO_RAD;
    const double dLat = (lat - homeLatitude) * DEG_TO_RAD;
    const double dLon = (lon - homeLongitude) * DEG_TO_RAD;
    const double a = sin(dLat / 2) * sin(dLat / 2) + cos(lat1) * cos(lat2) * sin(dLon / 2) * sin(dLon / 2);
    return static_cast<float>(EARTH_RADIUS_KM * 2 * atan2(sqrt(a), sqrt(1 - a)));
}

bool LiveSpotsService::isValid() const { return valid; }
bool LiveSpotsService::isUpdating() const { return updating; }
const BandSpotSummary& LiveSpotsService::getBand(uint8_t index) const { return bands[index]; }
const LiveSpot& LiveSpotsService::getRecentSpot(uint8_t index) const { return recentSpots[index]; }
uint8_t LiveSpotsService::getRecentSpotCount() const { return recentSpotCount; }
uint16_t LiveSpotsService::getTotalSpotCount() const { return totalSpotCount; }
const String& LiveSpotsService::getLastError() const { return lastError; }
const String& LiveSpotsService::getGridSquare() const { return gridSquare; }
bool LiveSpotsService::timeReached(unsigned long target) { return static_cast<long>(millis() - target) >= 0; }
