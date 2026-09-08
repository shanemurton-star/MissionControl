#pragma once

#include <Arduino.h>

#include "../models/AppSettings.h"
#include "../models/PotaSpotData.h"

class PotaService
{
public:
    static constexpr uint8_t MAX_SPOTS = 40;
    static constexpr uint8_t MAX_NEAREST_PARKS = 10;
    static constexpr float ACTIVE_RADIUS_MILES = 100.0f;
    static constexpr uint8_t MAP_TILE_COUNT = 9;
    static constexpr uint16_t MAP_VIEW_WIDTH = 368;
    static constexpr uint16_t MAP_VIEW_HEIGHT = 276;

    ~PotaService();

    void begin(const AppSettings& settings);
    void update();
    bool isValid() const;
    bool isUpdating() const;
    uint8_t getSpotCount() const;
    const PotaSpotData& getSpot(uint8_t index) const;
    uint8_t getNearestParkCount() const;
    const PotaSpotData& getNearestPark(uint8_t index) const;
    const String& getLastError() const;
    uint32_t getDataRevision() const;
    void requestMap();
    bool isMapLoading() const;
    uint8_t getMapTileCount() const;
    const uint8_t* getMapTileData(uint8_t index) const;
    size_t getMapTileSize(uint8_t index) const;
    int16_t getMapTileX(uint8_t index) const;
    int16_t getMapTileY(uint8_t index) const;
    uint32_t getMapGeneration() const;
    const String& getMapError() const;
    bool projectToMap(
        double latitude, double longitude, int16_t& x, int16_t& y) const;
    float getMapMilesPerPixel() const;

private:
    static constexpr uint32_t REFRESH_MS = 15UL * 60UL * 1000UL;
    static constexpr uint32_t RETRY_MS = 30UL * 1000UL;
    static constexpr uint32_t REQUEST_TIMEOUT_MS = 8000UL;
    static constexpr uint8_t MAP_ZOOM = 6;
    static constexpr size_t MAX_MAP_TILE_BYTES = 160UL * 1024UL;

    void fetchSpots();
    void prepareMapTiles();
    void fetchNextMapTile();
    void releaseMapTiles();
    void insertSpot(const PotaSpotData& spot);
    void insertNearestPark(const PotaSpotData& spot);
    float distanceMiles(double latitude, double longitude) const;
    float bearingDegrees(double latitude, double longitude) const;
    static bool timeReached(uint32_t target);

    double homeLatitude = 0.0;
    double homeLongitude = 0.0;
    PotaSpotData spots[MAX_SPOTS];
    PotaSpotData nearestParks[MAX_NEAREST_PARKS];
    uint8_t spotCount = 0;
    uint8_t nearestParkCount = 0;
    uint32_t nextUpdateMs = 0;
    uint32_t dataRevision = 0;
    bool valid = false;
    bool updating = false;
    String lastError;
    uint8_t* mapTileData[MAP_TILE_COUNT] = {};
    size_t mapTileSizes[MAP_TILE_COUNT] = {};
    int16_t mapTilePositionsX[MAP_TILE_COUNT] = {};
    int16_t mapTilePositionsY[MAP_TILE_COUNT] = {};
    int16_t mapTileServerX[MAP_TILE_COUNT] = {};
    int16_t mapTileServerY[MAP_TILE_COUNT] = {};
    uint8_t mapTileCount = 0;
    uint8_t nextMapTile = 0;
    uint32_t mapGeneration = 0;
    double mapLeftWorldPixel = 0.0;
    double mapTopWorldPixel = 0.0;
    double mapCenterWorldPixelX = 0.0;
    bool mapRequested = false;
    bool mapLoading = false;
    String mapError;
};
