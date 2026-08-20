#pragma once

#include <Arduino.h>

#include "../models/AppSettings.h"

class RadarService
{
public:
    ~RadarService();

    void begin(const AppSettings& settings);
    void update();

    bool isValid() const;
    bool hasBaseMap() const;
    bool isUpdating() const;

    const uint8_t* getImageData() const;
    size_t getImageSize() const;
    const uint8_t* getBaseMapData() const;
    size_t getBaseMapSize() const;

    uint16_t getLocationPixelX() const;
    uint16_t getLocationPixelY() const;
    uint32_t getFrameTime() const;
    uint32_t getGeneration() const;
    const String& getLastError() const;

private:
    enum class State
    {
        WaitingForWiFi,
        FetchBaseMap,
        FetchRadar,
        Ready,
        RetryDelay
    };

    static constexpr unsigned long REFRESH_INTERVAL_MS =
        5UL * 60UL * 1000UL;
    static constexpr unsigned long RETRY_INTERVAL_MS =
        60UL * 1000UL;
    static constexpr unsigned long REQUEST_TIMEOUT_MS =
        12000UL;
    static constexpr size_t MAX_IMAGE_SIZE =
        512UL * 1024UL;

    void configureMapRequests();
    void fetchBaseMap();
    void fetchRadar();

    bool downloadImage(
        const String& url,
        uint8_t*& destination,
        size_t& destinationSize,
        const char* description);

    void setError(const String& message);
    void scheduleRetry();
    void releaseImages();

    static bool timeReached(unsigned long targetTime);

    double latitude = 0.0;
    double longitude = 0.0;

    State state = State::WaitingForWiFi;
    String baseMapUrl;
    String radarUrl;
    String lastError;

    uint8_t* imageData = nullptr;
    size_t imageSize = 0;
    uint8_t* baseMapData = nullptr;
    size_t baseMapSize = 0;

    uint16_t locationPixelX = 128;
    uint16_t locationPixelY = 128;
    uint32_t frameTime = 0;
    uint32_t generation = 0;

    unsigned long nextActionMs = 0;
    bool updating = false;
};
