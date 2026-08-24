#include "SolarService.h"

#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <JPEGDEC.h>
#include <WiFi.h>
#include <esp_heap_caps.h>
#include <new>
#include <time.h>

namespace
{
    const char* BASE = "https://services.swpc.noaa.gov";
    const char* CORONA_URL =
        "https://services.swpc.noaa.gov/images/animations/lasco-c2/latest.jpg";

    class ImageBufferStream : public Stream
    {
    public:
        ImageBufferStream(uint8_t* buffer, size_t capacity)
            : buffer(buffer), capacity(capacity) {}

        size_t write(uint8_t value) override { return write(&value, 1); }
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

    struct CoronaDecodeContext
    {
        uint16_t* pixels = nullptr;
        uint16_t width = 0;
        uint16_t height = 0;
    };

    int drawCoronaBlock(JPEGDRAW* block)
    {
        CoronaDecodeContext* context =
            static_cast<CoronaDecodeContext*>(block->pUser);
        if (context == nullptr || context->pixels == nullptr) return 0;

        const int copyWidth = min(
            block->iWidthUsed > 0 ? block->iWidthUsed : block->iWidth,
            static_cast<int>(context->width) - block->x);
        const int copyHeight = min(
            block->iHeight,
            static_cast<int>(context->height) - block->y);
        if (copyWidth <= 0 || copyHeight <= 0) return 1;

        for (int row = 0; row < copyHeight; ++row)
        {
            memcpy(
                context->pixels + (block->y + row) * context->width + block->x,
                block->pPixels + row * block->iWidth,
                static_cast<size_t>(copyWidth) * sizeof(uint16_t));
        }
        return 1;
    }
}

SolarService::~SolarService()
{
    releaseCoronaImage();
}

void SolarService::begin()
{
    nextUpdateMs = 0;
    nextCoronaUpdateMs = 0;
    coronaError = "";
}

void SolarService::update(bool allowCoronaImage)
{
    const bool metricsDue = timeReached(nextUpdateMs);
    const bool coronaDue = allowCoronaImage && timeReached(nextCoronaUpdateMs);
    if (updating || (!metricsDue && !coronaDue) ||
        WiFi.status() != WL_CONNECTED) return;

    updating = true;
    lastError = "";

    if (metricsDue)
    {
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
    }

    // Keep the image on its own schedule so the small Arduino loopTask can
    // load startup metrics without constructing the JPEG decoder. The normal
    // background network worker performs this supplemental operation.
    if (coronaDue)
    {
        nextCoronaUpdateMs = millis() +
            (fetchCoronaImage() ? REFRESH_MS : RETRY_MS);
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

bool SolarService::fetchCoronaImage()
{
    coronaError = "";

    HTTPClient http;
    http.setConnectTimeout(REQUEST_TIMEOUT_MS);
    http.setTimeout(REQUEST_TIMEOUT_MS);
    http.setUserAgent("MissionControl-ESP32/1.0");
    if (!http.begin(CORONA_URL))
    {
        coronaError = "Unable to start corona request";
        return false;
    }

    Serial.println("[SolarService] Fetching NOAA LASCO C2 corona image");
    const int response = http.GET();
    if (response < 200 || response >= 300)
    {
        coronaError = String("Corona HTTP ") + response;
        http.end();
        Serial.println("[SolarService] " + coronaError);
        return false;
    }

    const int contentLength = http.getSize();
    if (contentLength > 0 &&
        static_cast<size_t>(contentLength) > MAX_CORONA_JPEG_SIZE)
    {
        coronaError = "Corona image is too large";
        http.end();
        return false;
    }

    const size_t capacity = contentLength > 0
        ? static_cast<size_t>(contentLength)
        : MAX_CORONA_JPEG_SIZE;
    uint8_t* jpegData = static_cast<uint8_t*>(heap_caps_malloc(
        capacity, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    if (jpegData == nullptr) jpegData = static_cast<uint8_t*>(malloc(capacity));
    if (jpegData == nullptr)
    {
        coronaError = "Not enough memory for corona download";
        http.end();
        return false;
    }

    ImageBufferStream imageStream(jpegData, capacity);
    const int bytesWritten = http.writeToStream(&imageStream);
    http.end();
    if (bytesWritten <= 0 || imageStream.getSize() == 0 ||
        imageStream.hasOverflowed() ||
        (contentLength > 0 && bytesWritten != contentLength))
    {
        free(jpegData);
        coronaError = imageStream.hasOverflowed()
            ? "Corona image exceeded buffer"
            : "Corona download was incomplete";
        return false;
    }

    // JPEGDEC contains roughly 22 KB of decoder state. Constructing it as a
    // local variable overflows Arduino's loopTask stack and leaves very little
    // headroom even on the network worker. Keep that state in PSRAM instead.
    void* decoderStorage = heap_caps_malloc(
        sizeof(JPEGDEC), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (decoderStorage == nullptr)
        decoderStorage = heap_caps_malloc(
            sizeof(JPEGDEC), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    if (decoderStorage == nullptr)
    {
        free(jpegData);
        coronaError = "Not enough memory for JPEG decoder";
        return false;
    }

    JPEGDEC* decoder = new (decoderStorage) JPEGDEC();
    if (!decoder->openRAM(
            jpegData, static_cast<int>(imageStream.getSize()), drawCoronaBlock))
    {
        decoder->~JPEGDEC();
        free(decoderStorage);
        free(jpegData);
        coronaError = "Unable to read corona JPEG";
        return false;
    }

    constexpr uint8_t scaleDivisor = 4;
    const uint16_t decodedWidth =
        static_cast<uint16_t>((decoder->getWidth() + scaleDivisor - 1) / scaleDivisor);
    const uint16_t decodedHeight =
        static_cast<uint16_t>((decoder->getHeight() + scaleDivisor - 1) / scaleDivisor);
    const size_t pixelBytes =
        static_cast<size_t>(decodedWidth) * decodedHeight * sizeof(uint16_t);
    uint16_t* decodedPixels = static_cast<uint16_t*>(heap_caps_malloc(
        pixelBytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    if (decodedPixels == nullptr)
        decodedPixels = static_cast<uint16_t*>(malloc(pixelBytes));
    if (decodedPixels == nullptr)
    {
        decoder->close();
        decoder->~JPEGDEC();
        free(decoderStorage);
        free(jpegData);
        coronaError = "Not enough memory to decode corona";
        return false;
    }

    CoronaDecodeContext context;
    context.pixels = decodedPixels;
    context.width = decodedWidth;
    context.height = decodedHeight;
    decoder->setUserPointer(&context);
    const int decodeResult = decoder->decode(0, 0, JPEG_SCALE_QUARTER);
    decoder->close();
    decoder->~JPEGDEC();
    free(decoderStorage);
    free(jpegData);
    if (!decodeResult)
    {
        free(decodedPixels);
        coronaError = "Corona JPEG decode failed";
        return false;
    }

    if (coronaPixels != nullptr) free(coronaPixels);
    coronaPixels = decodedPixels;
    coronaWidth = decodedWidth;
    coronaHeight = decodedHeight;
    ++coronaGeneration;

    Serial.print("[SolarService] Corona image ready: ");
    Serial.print(coronaWidth);
    Serial.print("x");
    Serial.println(coronaHeight);
    return true;
}

bool SolarService::isValid() const { return valid; }
bool SolarService::isUpdating() const { return updating; }
const SolarData& SolarService::getData() const { return data; }
const String& SolarService::getLastError() const { return lastError; }
bool SolarService::hasCoronaImage() const { return coronaPixels != nullptr; }
const uint16_t* SolarService::getCoronaPixels() const { return coronaPixels; }
uint16_t SolarService::getCoronaWidth() const { return coronaWidth; }
uint16_t SolarService::getCoronaHeight() const { return coronaHeight; }
uint32_t SolarService::getCoronaGeneration() const { return coronaGeneration; }
const String& SolarService::getCoronaError() const { return coronaError; }

void SolarService::releaseCoronaImage()
{
    if (coronaPixels != nullptr)
    {
        free(coronaPixels);
        coronaPixels = nullptr;
    }
    coronaWidth = 0;
    coronaHeight = 0;
}

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
