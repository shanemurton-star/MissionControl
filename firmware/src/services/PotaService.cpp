#include "PotaService.h"

#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <esp_heap_caps.h>
#include <math.h>

namespace
{
    constexpr double EARTH_RADIUS_MILES = 3958.7613;
    const char* POTA_SPOTS_URL = "https://api.pota.app/spot/activator";

    class BoundedImageStream : public Stream
    {
    public:
        BoundedImageStream(uint8_t* target, size_t capacity)
            : target(target), capacity(capacity) {}

        size_t write(uint8_t value) override { return write(&value, 1); }
        size_t write(const uint8_t* source, size_t length) override
        {
            if (length > capacity - size)
            {
                overflow = true;
                return 0;
            }
            memcpy(target + size, source, length);
            size += length;
            return length;
        }
        int available() override { return 0; }
        int read() override { return -1; }
        int peek() override { return -1; }
        void flush() override {}
        size_t length() const { return size; }
        bool overflowed() const { return overflow; }

    private:
        uint8_t* target;
        size_t capacity;
        size_t size = 0;
        bool overflow = false;
    };

    double longitudeToWorldPixel(double longitude, uint8_t zoom)
    {
        const double scale = static_cast<double>(256UL << zoom);
        return (longitude + 180.0) / 360.0 * scale;
    }

    double latitudeToWorldPixel(double latitude, uint8_t zoom)
    {
        const double bounded = constrain(latitude, -85.05112878, 85.05112878);
        const double radians = bounded * DEG_TO_RAD;
        const double scale = static_cast<double>(256UL << zoom);
        return (1.0 - asinh(tan(radians)) / PI) * 0.5 * scale;
    }
}

PotaService::~PotaService()
{
    releaseMapTiles();
}

void PotaService::begin(const AppSettings& settings)
{
    const bool locationChanged =
        fabs(homeLatitude - settings.latitude) > 0.00001 ||
        fabs(homeLongitude - settings.longitude) > 0.00001;
    homeLatitude = settings.latitude;
    homeLongitude = settings.longitude;
    if (locationChanged)
    {
        releaseMapTiles();
        mapRequested = false;
        mapGeneration = 0;
    }
    nextUpdateMs = 0;
    dataRevision = 0;
}

void PotaService::update()
{
    if (updating || WiFi.status() != WL_CONNECTED) return;
    if (mapRequested && nextMapTile < mapTileCount)
    {
        fetchNextMapTile();
        return;
    }
    if (!timeReached(nextUpdateMs)) return;
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
    nearestParkCount = 0;
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
        spot.latitude = latitude;
        spot.longitude = longitude;
        spot.distanceMiles = distanceMiles(latitude, longitude);
        spot.bearingDegrees = bearingDegrees(latitude, longitude);
        insertNearestPark(spot);
        if (spot.distanceMiles > ACTIVE_RADIUS_MILES) continue;
        insertSpot(spot);
    }

    valid = true;
    ++dataRevision;
    updating = false;
    nextUpdateMs = millis() + REFRESH_MS;
}

void PotaService::prepareMapTiles()
{
    releaseMapTiles();
    mapError = "";
    mapCenterWorldPixelX = longitudeToWorldPixel(homeLongitude, MAP_ZOOM);
    const double centerY = latitudeToWorldPixel(homeLatitude, MAP_ZOOM);
    mapLeftWorldPixel = floor(
        mapCenterWorldPixelX - static_cast<double>(MAP_VIEW_WIDTH) / 2.0);
    mapTopWorldPixel = floor(
        centerY - static_cast<double>(MAP_VIEW_HEIGHT) / 2.0);

    const int16_t firstTileX =
        static_cast<int16_t>(floor(mapLeftWorldPixel / 256.0));
    const int16_t lastTileX = static_cast<int16_t>(floor(
        (mapLeftWorldPixel + MAP_VIEW_WIDTH - 1.0) / 256.0));
    const int16_t firstTileY =
        static_cast<int16_t>(floor(mapTopWorldPixel / 256.0));
    const int16_t lastTileY = static_cast<int16_t>(floor(
        (mapTopWorldPixel + MAP_VIEW_HEIGHT - 1.0) / 256.0));
    const int16_t tileLimit = 1 << MAP_ZOOM;

    mapTileCount = 0;
    for (int16_t tileY = firstTileY;
         tileY <= lastTileY && mapTileCount < MAP_TILE_COUNT; ++tileY)
    {
        if (tileY < 0 || tileY >= tileLimit) continue;
        for (int16_t tileX = firstTileX;
             tileX <= lastTileX && mapTileCount < MAP_TILE_COUNT; ++tileX)
        {
            int16_t wrappedX = tileX % tileLimit;
            if (wrappedX < 0) wrappedX += tileLimit;
            mapTileServerX[mapTileCount] = wrappedX;
            mapTileServerY[mapTileCount] = tileY;
            mapTilePositionsX[mapTileCount] = static_cast<int16_t>(
                tileX * 256.0 - mapLeftWorldPixel);
            mapTilePositionsY[mapTileCount] = static_cast<int16_t>(
                tileY * 256.0 - mapTopWorldPixel);
            ++mapTileCount;
        }
    }
    nextMapTile = 0;
    mapLoading = mapTileCount > 0;
}

void PotaService::fetchNextMapTile()
{
    if (nextMapTile >= mapTileCount)
    {
        mapLoading = false;
        return;
    }

    updating = true;
    const uint8_t index = nextMapTile++;
    const String url = String("https://tile.openstreetmap.org/") + MAP_ZOOM +
        "/" + mapTileServerX[index] + "/" + mapTileServerY[index] + ".png";
    WiFiClientSecure client;
    client.setInsecure();
    HTTPClient http;
    http.setConnectTimeout(REQUEST_TIMEOUT_MS);
    http.setTimeout(REQUEST_TIMEOUT_MS);
    http.setUserAgent(
        "MissionControl-ESP32/1.1.0 (+https://github.com/shanemurton-star/MissionControl)");

    Serial.print("[PotaService] Fetching map tile ");
    Serial.print(index + 1);
    Serial.print("/");
    Serial.println(mapTileCount);
    if (!http.begin(client, url))
    {
        mapError = "UNABLE TO START MAP REQUEST";
        updating = false;
        mapLoading = nextMapTile < mapTileCount;
        return;
    }

    const int response = http.GET();
    if (response < 200 || response >= 300)
    {
        mapError = String("MAP TILE HTTP ") + response;
        http.end();
        updating = false;
        mapLoading = nextMapTile < mapTileCount;
        return;
    }
    const int contentLength = http.getSize();
    if (contentLength > 0 &&
        static_cast<size_t>(contentLength) > MAX_MAP_TILE_BYTES)
    {
        mapError = "MAP TILE TOO LARGE";
        http.end();
        updating = false;
        mapLoading = nextMapTile < mapTileCount;
        return;
    }

    const size_t capacity = contentLength > 0
        ? static_cast<size_t>(contentLength) : MAX_MAP_TILE_BYTES;
    uint8_t* data = static_cast<uint8_t*>(heap_caps_malloc(
        capacity, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    if (data == nullptr) data = static_cast<uint8_t*>(malloc(capacity));
    if (data == nullptr)
    {
        mapError = "NOT ENOUGH MEMORY FOR MAP";
        http.end();
        updating = false;
        mapLoading = nextMapTile < mapTileCount;
        return;
    }

    BoundedImageStream stream(data, capacity);
    const int bytesWritten = http.writeToStream(&stream);
    http.end();
    if (bytesWritten <= 0 || stream.length() == 0 || stream.overflowed())
    {
        free(data);
        mapError = "MAP TILE DOWNLOAD FAILED";
    }
    else
    {
        mapTileData[index] = data;
        mapTileSizes[index] = stream.length();
        ++mapGeneration;
    }
    mapLoading = nextMapTile < mapTileCount;
    updating = false;
}

void PotaService::releaseMapTiles()
{
    for (uint8_t index = 0; index < MAP_TILE_COUNT; ++index)
    {
        if (mapTileData[index] != nullptr) free(mapTileData[index]);
        mapTileData[index] = nullptr;
        mapTileSizes[index] = 0;
    }
    mapTileCount = 0;
    nextMapTile = 0;
    mapLoading = false;
}

void PotaService::insertNearestPark(const PotaSpotData& spot)
{
    for (uint8_t index = 0; index < nearestParkCount; ++index)
        if (nearestParks[index].reference == spot.reference) return;

    uint8_t position = 0;
    while (position < nearestParkCount &&
           nearestParks[position].distanceMiles <= spot.distanceMiles)
        ++position;
    if (position >= MAX_NEAREST_PARKS) return;

    const uint8_t last = nearestParkCount < MAX_NEAREST_PARKS
        ? nearestParkCount : MAX_NEAREST_PARKS - 1;
    for (uint8_t index = last; index > position; --index)
        nearestParks[index] = nearestParks[index - 1];
    nearestParks[position] = spot;
    if (nearestParkCount < MAX_NEAREST_PARKS) ++nearestParkCount;
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

float PotaService::bearingDegrees(double latitude, double longitude) const
{
    const double lat1 = homeLatitude * DEG_TO_RAD;
    const double lat2 = latitude * DEG_TO_RAD;
    const double deltaLongitude = (longitude - homeLongitude) * DEG_TO_RAD;
    const double y = sin(deltaLongitude) * cos(lat2);
    const double x = cos(lat1) * sin(lat2) -
        sin(lat1) * cos(lat2) * cos(deltaLongitude);
    double bearing = atan2(y, x) / DEG_TO_RAD;
    if (bearing < 0.0) bearing += 360.0;
    return static_cast<float>(bearing);
}

bool PotaService::isValid() const { return valid; }
bool PotaService::isUpdating() const { return updating; }
uint8_t PotaService::getSpotCount() const { return spotCount; }
const PotaSpotData& PotaService::getSpot(uint8_t index) const { return spots[index]; }
uint8_t PotaService::getNearestParkCount() const { return nearestParkCount; }
const PotaSpotData& PotaService::getNearestPark(uint8_t index) const
{
    return nearestParks[index];
}
const String& PotaService::getLastError() const { return lastError; }
uint32_t PotaService::getDataRevision() const { return dataRevision; }
void PotaService::requestMap()
{
    if (mapTileCount > 0 && nextMapTile >= mapTileCount) return;
    mapRequested = true;
    prepareMapTiles();
}
bool PotaService::isMapLoading() const { return mapLoading; }
uint8_t PotaService::getMapTileCount() const { return mapTileCount; }
const uint8_t* PotaService::getMapTileData(uint8_t index) const
{
    return index < mapTileCount ? mapTileData[index] : nullptr;
}
size_t PotaService::getMapTileSize(uint8_t index) const
{
    return index < mapTileCount ? mapTileSizes[index] : 0;
}
int16_t PotaService::getMapTileX(uint8_t index) const
{
    return index < mapTileCount ? mapTilePositionsX[index] : 0;
}
int16_t PotaService::getMapTileY(uint8_t index) const
{
    return index < mapTileCount ? mapTilePositionsY[index] : 0;
}
uint32_t PotaService::getMapGeneration() const { return mapGeneration; }
const String& PotaService::getMapError() const { return mapError; }
bool PotaService::projectToMap(
    double latitude, double longitude, int16_t& x, int16_t& y) const
{
    if (mapTileCount == 0) return false;
    double worldX = longitudeToWorldPixel(longitude, MAP_ZOOM);
    const double worldSize = static_cast<double>(256UL << MAP_ZOOM);
    while (worldX - mapCenterWorldPixelX > worldSize / 2.0) worldX -= worldSize;
    while (mapCenterWorldPixelX - worldX > worldSize / 2.0) worldX += worldSize;
    const double worldY = latitudeToWorldPixel(latitude, MAP_ZOOM);
    x = static_cast<int16_t>(round(worldX - mapLeftWorldPixel));
    y = static_cast<int16_t>(round(worldY - mapTopWorldPixel));
    return x >= 0 && x < MAP_VIEW_WIDTH && y >= 0 && y < MAP_VIEW_HEIGHT;
}
float PotaService::getMapMilesPerPixel() const
{
    const double circumferenceMiles = 2.0 * PI * EARTH_RADIUS_MILES;
    return static_cast<float>(
        circumferenceMiles * cos(homeLatitude * DEG_TO_RAD) /
        static_cast<double>(256UL << MAP_ZOOM));
}
bool PotaService::timeReached(uint32_t target)
{
    return static_cast<int32_t>(millis() - target) >= 0;
}
