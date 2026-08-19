#include "RadarService.h"

#include <HTTPClient.h>
#include <WiFi.h>
#include <esp_heap_caps.h>
#include <math.h>
#include <time.h>

namespace
{
    const char* USER_AGENT =
        "MissionControl-ESP32/1.0";

    constexpr uint8_t MAP_ZOOM = 6;
    constexpr uint16_t TILE_SIZE = 256;
    constexpr double WEB_MERCATOR_ORIGIN =
        20037508.342789244;

    class ImageBufferStream : public Stream
    {
    public:
        ImageBufferStream(uint8_t* buffer, size_t capacity)
            : buffer(buffer), capacity(capacity)
        {
        }

        size_t write(uint8_t value) override
        {
            return write(&value, 1);
        }

        size_t write(const uint8_t* data, size_t length) override
        {
            if (length > capacity - size)
            {
                overflowed = true;
                return 0;
            }

            memcpy(buffer + size, data, length);
            size += length;
            return length;
        }

        int available() override { return 0; }
        int read() override { return -1; }
        int peek() override { return -1; }
        void flush() override {}

        size_t getSize() const { return size; }
        bool hasOverflowed() const { return overflowed; }

    private:
        uint8_t* buffer;
        size_t capacity;
        size_t size = 0;
        bool overflowed = false;
    };
}

RadarService::~RadarService()
{
    releaseImages();
}

void RadarService::begin(const AppSettings& settings)
{
    latitude = settings.latitude;
    longitude = settings.longitude;

    lastError = "";
    frameTime = 0;
    generation = 0;
    nextActionMs = 0;
    updating = false;

    releaseImages();
    configureMapRequests();

    state = WiFi.status() == WL_CONNECTED
        ? State::FetchBaseMap
        : State::WaitingForWiFi;
}

void RadarService::update()
{
    switch (state)
    {
        case State::WaitingForWiFi:
            if (WiFi.status() == WL_CONNECTED)
            {
                state = hasBaseMap()
                    ? State::FetchRadar
                    : State::FetchBaseMap;
            }
            break;

        case State::FetchBaseMap:
            fetchBaseMap();
            break;

        case State::FetchRadar:
            fetchRadar();
            break;

        case State::Ready:
            if (timeReached(nextActionMs))
            {
                state = State::FetchRadar;
            }
            break;

        case State::RetryDelay:
            if (timeReached(nextActionMs))
            {
                state = WiFi.status() != WL_CONNECTED
                    ? State::WaitingForWiFi
                    : hasBaseMap()
                        ? State::FetchRadar
                        : State::FetchBaseMap;
            }
            break;
    }
}

bool RadarService::isValid() const
{
    return imageData != nullptr && imageSize > 0;
}

bool RadarService::hasBaseMap() const
{
    return baseMapData != nullptr && baseMapSize > 0;
}

bool RadarService::isUpdating() const
{
    return updating;
}

const uint8_t* RadarService::getImageData() const
{
    return imageData;
}

size_t RadarService::getImageSize() const
{
    return imageSize;
}

const uint8_t* RadarService::getBaseMapData() const
{
    return baseMapData;
}

size_t RadarService::getBaseMapSize() const
{
    return baseMapSize;
}

uint16_t RadarService::getLocationPixelX() const
{
    return locationPixelX;
}

uint16_t RadarService::getLocationPixelY() const
{
    return locationPixelY;
}

uint32_t RadarService::getFrameTime() const
{
    return frameTime;
}

uint32_t RadarService::getGeneration() const
{
    return generation;
}

const String& RadarService::getLastError() const
{
    return lastError;
}

void RadarService::configureMapRequests()
{
    constexpr uint32_t tileCount = 1UL << MAP_ZOOM;

    const double worldX =
        (longitude + 180.0) / 360.0 * tileCount;

    const double latitudeRadians =
        latitude * PI / 180.0;

    const double worldY =
        (1.0 -
         asinh(tan(latitudeRadians)) / PI) /
        2.0 * tileCount;

    const uint32_t tileX =
        static_cast<uint32_t>(floor(worldX));
    const uint32_t tileY =
        static_cast<uint32_t>(floor(worldY));

    locationPixelX =
        static_cast<uint16_t>(
            (worldX - tileX) * TILE_SIZE);
    locationPixelY =
        static_cast<uint16_t>(
            (worldY - tileY) * TILE_SIZE);

    baseMapUrl =
        String("https://a.basemaps.cartocdn.com/dark_all/") +
        String(MAP_ZOOM) + "/" +
        String(tileX) + "/" +
        String(tileY) + ".png";

    const double tileSpan =
        WEB_MERCATOR_ORIGIN * 2.0 / tileCount;
    const double minimumX =
        -WEB_MERCATOR_ORIGIN + tileX * tileSpan;
    const double maximumX = minimumX + tileSpan;
    const double maximumY =
        WEB_MERCATOR_ORIGIN - tileY * tileSpan;
    const double minimumY = maximumY - tileSpan;

    radarUrl =
        "https://mesonet.agron.iastate.edu/cgi-bin/"
        "wms/nexrad/n0q.cgi?SERVICE=WMS&VERSION=1.1.1"
        "&REQUEST=GetMap&LAYERS=nexrad-n0q-900913"
        "&STYLES=&SRS=EPSG:3857&BBOX=" +
        String(minimumX, 3) + "," +
        String(minimumY, 3) + "," +
        String(maximumX, 3) + "," +
        String(maximumY, 3) +
        "&WIDTH=256&HEIGHT=256&FORMAT=image/png"
        "&TRANSPARENT=TRUE";
}

void RadarService::fetchBaseMap()
{
    updating = true;
    lastError = "";

    if (!downloadImage(
            baseMapUrl,
            baseMapData,
            baseMapSize,
            "basemap"))
    {
        scheduleRetry();
        return;
    }

    state = State::FetchRadar;
}

void RadarService::fetchRadar()
{
    updating = true;
    lastError = "";

    if (!downloadImage(
            radarUrl,
            imageData,
            imageSize,
            "IEM NEXRAD radar"))
    {
        scheduleRetry();
        return;
    }

    frameTime = static_cast<uint32_t>(time(nullptr));
    generation++;
    updating = false;
    state = State::Ready;
    nextActionMs = millis() + REFRESH_INTERVAL_MS;

    Serial.print("[RadarService] Map ready: ");
    Serial.print(baseMapSize);
    Serial.print(" + ");
    Serial.print(imageSize);
    Serial.println(" bytes");
}

bool RadarService::downloadImage(
    const String& url,
    uint8_t*& destination,
    size_t& destinationSize,
    const char* description)
{
    if (WiFi.status() != WL_CONNECTED)
    {
        setError("Wi-Fi is not connected");
        state = State::WaitingForWiFi;
        return false;
    }

    HTTPClient http;
    http.setConnectTimeout(REQUEST_TIMEOUT_MS);
    http.setTimeout(REQUEST_TIMEOUT_MS);

    if (!http.begin(url))
    {
        setError(String("Unable to initialize ") + description);
        return false;
    }

    http.addHeader("User-Agent", USER_AGENT);

    Serial.print("[RadarService] Fetching ");
    Serial.println(description);

    const int responseCode = http.GET();

    if (responseCode < 200 || responseCode >= 300)
    {
        setError(
            String(description) + " returned HTTP " +
            String(responseCode));
        http.end();
        return false;
    }

    const int contentLength = http.getSize();

    if (contentLength > 0 &&
        static_cast<size_t>(contentLength) > MAX_IMAGE_SIZE)
    {
        setError(String(description) + " is larger than 512 KB");
        http.end();
        return false;
    }

    const size_t bufferCapacity = contentLength > 0
        ? static_cast<size_t>(contentLength)
        : MAX_IMAGE_SIZE;

    uint8_t* newImage =
        static_cast<uint8_t*>(
            heap_caps_malloc(
                bufferCapacity,
                MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));

    if (newImage == nullptr)
    {
        newImage = static_cast<uint8_t*>(malloc(bufferCapacity));
    }

    if (newImage == nullptr)
    {
        setError(String("Not enough memory for ") + description);
        http.end();
        return false;
    }

    ImageBufferStream imageStream(newImage, bufferCapacity);
    const int bytesWritten = http.writeToStream(&imageStream);

    http.end();

    if (bytesWritten <= 0 ||
        imageStream.getSize() == 0 ||
        imageStream.hasOverflowed() ||
        (contentLength > 0 && bytesWritten != contentLength))
    {
        free(newImage);
        setError(
            imageStream.hasOverflowed()
                ? String(description) + " is larger than 512 KB"
                : String(description) + " download was incomplete");
        return false;
    }

    if (destination != nullptr)
    {
        free(destination);
    }

    destination = newImage;
    destinationSize = imageStream.getSize();
    return true;
}

void RadarService::setError(const String& message)
{
    lastError = message;
    updating = false;
    Serial.print("[RadarService] Error: ");
    Serial.println(message);
}

void RadarService::scheduleRetry()
{
    state = State::RetryDelay;
    nextActionMs = millis() + RETRY_INTERVAL_MS;
}

void RadarService::releaseImages()
{
    if (imageData != nullptr)
    {
        free(imageData);
        imageData = nullptr;
    }

    if (baseMapData != nullptr)
    {
        free(baseMapData);
        baseMapData = nullptr;
    }

    imageSize = 0;
    baseMapSize = 0;
}

bool RadarService::timeReached(unsigned long targetTime)
{
    if (targetTime == 0)
    {
        return true;
    }

    return static_cast<long>(millis() - targetTime) >= 0;
}
