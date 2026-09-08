#include "WsjtxService.h"

#include <HTTPClient.h>
#include <JPEGDEC.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <esp_heap_caps.h>
#include <math.h>
#include <new>

namespace
{
    class BoundedImageStream : public Stream
    {
    public:
        BoundedImageStream(uint8_t* target, size_t capacity)
            : target(target), capacity(capacity) {}
        size_t write(uint8_t value) override { return write(&value, 1); }
        size_t write(const uint8_t* source, size_t length) override
        {
            if (length > capacity - size) { overflow = true; return 0; }
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

    struct PhotoDecodeContext
    {
        uint16_t* pixels = nullptr;
        uint16_t width = 0;
        uint16_t height = 0;
    };

    int drawPhotoBlock(JPEGDRAW* block)
    {
        PhotoDecodeContext* context =
            static_cast<PhotoDecodeContext*>(block->pUser);
        if (context == nullptr || context->pixels == nullptr) return 0;
        const int copyWidth = min(
            block->iWidthUsed > 0 ? block->iWidthUsed : block->iWidth,
            static_cast<int>(context->width) - block->x);
        const int copyHeight = min(
            block->iHeight, static_cast<int>(context->height) - block->y);
        if (copyWidth <= 0 || copyHeight <= 0) return 1;
        for (int row = 0; row < copyHeight; ++row)
            memcpy(
                context->pixels + (block->y + row) * context->width + block->x,
                block->pPixels + row * block->iWidth,
                static_cast<size_t>(copyWidth) * sizeof(uint16_t));
        return 1;
    }

    class PacketReader
    {
    public:
        PacketReader(const uint8_t* data, size_t length) : data(data), length(length) {}

        bool readU8(uint8_t& value)
        {
            if (position + 1 > length) return false;
            value = data[position++];
            return true;
        }

        bool readU32(uint32_t& value)
        {
            if (position + 4 > length) return false;
            value = (static_cast<uint32_t>(data[position]) << 24) |
                (static_cast<uint32_t>(data[position + 1]) << 16) |
                (static_cast<uint32_t>(data[position + 2]) << 8) |
                data[position + 3];
            position += 4;
            return true;
        }

        bool readU64(uint64_t& value)
        {
            uint32_t high = 0;
            uint32_t low = 0;
            if (!readU32(high) || !readU32(low)) return false;
            value = (static_cast<uint64_t>(high) << 32) | low;
            return true;
        }

        bool readByteArray(String& value)
        {
            uint32_t count = 0;
            if (!readU32(count)) return false;
            if (count == 0xFFFFFFFFUL) { value = ""; return true; }
            if (count > 512 || position + count > length) return false;
            value = "";
            value.reserve(count);
            for (uint32_t index = 0; index < count; ++index)
                value += static_cast<char>(data[position + index]);
            position += count;
            return true;
        }

    private:
        const uint8_t* data;
        size_t length;
        size_t position = 0;
    };

    String joinLocation(const String& city, const String& state)
    {
        if (city.isEmpty()) return state;
        if (state.isEmpty()) return city;
        return city + ", " + state;
    }
}

WsjtxService::~WsjtxService()
{
    releaseImages();
}

void WsjtxService::begin(const AppSettings& settings)
{
    configure(settings);
}

void WsjtxService::configure(const AppSettings& settings)
{
    udp.stop();
    configuration = settings;
    providerName = configuration.callsignLookupProvider == 1 ? "QRZ" : "HAMQTH";
    listening = false;
    listenerRetryMs = 0;
    sessionKey = "";
    releaseImages();
    lookupPending = !callsign.isEmpty();
    lookupRetryMs = 0;
    lookupStatus = callsign.isEmpty()
        ? "WAITING FOR A SELECTED CONTACT"
        : "CALLBOOK SETTINGS UPDATED";
    ++dataRevision;
}

bool WsjtxService::startListener()
{
    if (WiFi.status() != WL_CONNECTED) return false;
    IPAddress group;
    if (!group.fromString(configuration.wsjtxMulticastAddress)) return false;
    listening = udp.beginMulticast(group, configuration.wsjtxUdpPort) == 1;
    Serial.print("[WSJT-X] UDP listener ");
    Serial.print(listening ? "ready at " : "failed at ");
    Serial.print(configuration.wsjtxMulticastAddress);
    Serial.print(":");
    Serial.println(configuration.wsjtxUdpPort);
    return listening;
}

void WsjtxService::poll()
{
    if (WiFi.status() != WL_CONNECTED)
    {
        if (listening) udp.stop();
        listening = false;
        return;
    }
    if (!listening)
    {
        if (static_cast<int32_t>(millis() - listenerRetryMs) < 0) return;
        listenerRetryMs = millis() + 10000UL;
        if (!startListener()) return;
    }

    const int packetSize = udp.parsePacket();
    if (packetSize <= 0) return;
    if (packetSize > MAX_PACKET_SIZE)
    {
        while (udp.available()) udp.read();
        return;
    }
    const int received = udp.read(packetBuffer, packetSize);
    if (received > 0) parsePacket(packetBuffer, static_cast<size_t>(received));
}

void WsjtxService::parsePacket(const uint8_t* data, size_t length)
{
    PacketReader reader(data, length);
    uint32_t magic = 0;
    uint32_t schema = 0;
    uint32_t type = 0;
    String id;
    if (!reader.readU32(magic) || magic != WSJTX_MAGIC ||
        !reader.readU32(schema) || !reader.readU32(type) ||
        !reader.readByteArray(id)) return;
    (void)schema;
    (void)id;
    if (type != 1) return;

    uint64_t newFrequency = 0;
    String newMode;
    String newCallsign;
    String report;
    String txMode;
    uint8_t txEnabled = 0;
    uint8_t transmitting = 0;
    uint8_t decoding = 0;
    uint32_t rxDf = 0;
    uint32_t txDf = 0;
    String deCall;
    String deGrid;
    String newGrid;
    if (!reader.readU64(newFrequency) ||
        !reader.readByteArray(newMode) ||
        !reader.readByteArray(newCallsign) ||
        !reader.readByteArray(report) ||
        !reader.readByteArray(txMode) ||
        !reader.readU8(txEnabled) || !reader.readU8(transmitting) ||
        !reader.readU8(decoding) || !reader.readU32(rxDf) ||
        !reader.readU32(txDf) || !reader.readByteArray(deCall) ||
        !reader.readByteArray(deGrid) || !reader.readByteArray(newGrid)) return;
    (void)report;
    (void)txMode;
    (void)rxDf;
    (void)txDf;
    (void)deCall;
    (void)deGrid;

    newCallsign.trim();
    newCallsign.toUpperCase();
    const bool callChanged = !newCallsign.isEmpty() && newCallsign != callsign;
    dialFrequency = newFrequency;
    mode = newMode;
    grid = newGrid;
    if (transmitting) state = "TRANSMITTING";
    else if (txEnabled) state = "CALL ENABLED";
    else if (decoding) state = "DECODING";
    else state = "MONITORING";
    lastPacketMs = millis();

    if (callChanged)
    {
        callsign = newCallsign;
        callbookValid = false;
        callbookName = callbookLocation = callbookCountry = callbookGrid = "";
        photoUrl = countryCode = "";
        photoCallsign = flagCallsign = "";
        ++photoGeneration;
        ++flagGeneration;
        lookupStatus = "LOOKUP QUEUED";
        lookupPending = true;
        lookupRetryMs = 0;
    }
    ++dataRevision;
}

void WsjtxService::updateLookup()
{
    if (lookupRunning || !lookupPending || callsign.isEmpty() ||
        WiFi.status() != WL_CONNECTED ||
        static_cast<int32_t>(millis() - lookupRetryMs) < 0) return;
    if (configuration.callsignLookupUsername.isEmpty() ||
        configuration.callsignLookupPassword.isEmpty())
    {
        lookupStatus = providerName + " CREDENTIALS NOT CONFIGURED";
        lookupPending = false;
        ++dataRevision;
        return;
    }

    lookupRunning = true;
    lookupStatus = "LOOKING UP " + callsign + " ON " + providerName;
    ++dataRevision;
    const String target = callsign;
    const bool success = configuration.callsignLookupProvider == 1
        ? lookupQrz(target) : lookupHamQth(target);
    if (success && target == callsign)
    {
        if (!photoUrl.isEmpty()) fetchPhoto(target);
        if (!countryCode.isEmpty()) fetchFlag(target);
    }
    lookupRunning = false;
    lookupPending = !success;
    lookupRetryMs = millis() + LOOKUP_RETRY_MS;
    ++dataRevision;
}

bool WsjtxService::httpGet(const String& url, String& response, int& responseCode)
{
    WiFiClientSecure client;
    client.setInsecure();
    HTTPClient http;
    http.setConnectTimeout(8000);
    http.setTimeout(8000);
    http.setUserAgent("MissionControl-ESP32/1.0.2");
    if (!http.begin(client, url))
    {
        responseCode = -1;
        return false;
    }
    responseCode = http.GET();
    if (responseCode >= 200 && responseCode < 300)
        response = http.getString();
    http.end();
    return responseCode >= 200 && responseCode < 300;
}

bool WsjtxService::lookupHamQth(const String& targetCallsign)
{
    int code = 0;
    String xml;
    if (sessionKey.isEmpty())
    {
        const String loginUrl = "https://www.hamqth.com/xml.php?u=" +
            urlEncode(configuration.callsignLookupUsername) + "&p=" +
            urlEncode(configuration.callsignLookupPassword);
        if (!httpGet(loginUrl, xml, code))
        {
            lookupStatus = "HAMQTH LOGIN HTTP " + String(code);
            return false;
        }
        sessionKey = xmlValue(xml, "session_id");
        if (sessionKey.isEmpty())
        {
            lookupStatus = xmlValue(xml, "error");
            if (lookupStatus.isEmpty()) lookupStatus = "HAMQTH LOGIN FAILED";
            return false;
        }
    }

    const String lookupUrl = "https://www.hamqth.com/xml.php?id=" +
        urlEncode(sessionKey) + "&callsign=" + urlEncode(targetCallsign) +
        "&prg=MissionControl";
    xml = "";
    if (!httpGet(lookupUrl, xml, code))
    {
        lookupStatus = "HAMQTH LOOKUP HTTP " + String(code);
        return false;
    }
    const String error = xmlValue(xml, "error");
    if (!error.isEmpty())
    {
        lookupStatus = error;
        if (error.indexOf("Session") >= 0 || error.indexOf("session") >= 0)
            sessionKey = "";
        return false;
    }
    const String first = xmlValue(xml, "nick");
    const String full = xmlValue(xml, "adr_name");
    callbookName = full.isEmpty() ? first : full;
    callbookLocation = joinLocation(
        xmlValue(xml, "adr_city"), xmlValue(xml, "us_state"));
    if (callbookLocation.isEmpty()) callbookLocation = xmlValue(xml, "qth");
    callbookCountry = xmlValue(xml, "country");
    callbookGrid = xmlValue(xml, "grid");
    photoUrl = xmlValue(xml, "picture");
    if (photoUrl.indexOf("/images/default/") >= 0)
    {
        Serial.println("[WSJT-X] Ignoring HamQTH default profile banner");
        photoUrl = "";
    }
    countryCode = isoCountryCode(callbookCountry);
    callbookValid = true;
    lookupPending = false;
    lookupStatus = "HAMQTH LOOKUP COMPLETE";
    return true;
}

bool WsjtxService::lookupQrz(const String& targetCallsign)
{
    int code = 0;
    String xml;
    if (sessionKey.isEmpty())
    {
        const String loginUrl = "https://xmldata.qrz.com/xml/current/?username=" +
            urlEncode(configuration.callsignLookupUsername) + ";password=" +
            urlEncode(configuration.callsignLookupPassword) +
            ";agent=MissionControl-1.0.2";
        if (!httpGet(loginUrl, xml, code))
        {
            lookupStatus = "QRZ LOGIN HTTP " + String(code);
            return false;
        }
        sessionKey = xmlValue(xml, "Key");
        if (sessionKey.isEmpty())
        {
            lookupStatus = xmlValue(xml, "Error");
            if (lookupStatus.isEmpty()) lookupStatus = "QRZ XML ACCESS UNAVAILABLE";
            return false;
        }
    }

    const String lookupUrl = "https://xmldata.qrz.com/xml/current/?s=" +
        urlEncode(sessionKey) + ";callsign=" + urlEncode(targetCallsign);
    xml = "";
    if (!httpGet(lookupUrl, xml, code))
    {
        lookupStatus = "QRZ LOOKUP HTTP " + String(code);
        return false;
    }
    const String error = xmlValue(xml, "Error");
    if (!error.isEmpty())
    {
        lookupStatus = error;
        if (error.indexOf("Session") >= 0 || error.indexOf("session") >= 0)
            sessionKey = "";
        return false;
    }
    callbookName = joinLocation(xmlValue(xml, "fname"), xmlValue(xml, "name"));
    callbookLocation = joinLocation(xmlValue(xml, "addr2"), xmlValue(xml, "state"));
    callbookCountry = xmlValue(xml, "country");
    callbookGrid = xmlValue(xml, "grid");
    photoUrl = xmlValue(xml, "image");
    countryCode = isoCountryCode(callbookCountry);
    callbookValid = true;
    lookupPending = false;
    lookupStatus = "QRZ LOOKUP COMPLETE";
    return true;
}

bool WsjtxService::downloadImage(
    const String& url, size_t maximumBytes,
    uint8_t*& data, size_t& size, String& error)
{
    data = nullptr;
    size = 0;
    WiFiClientSecure client;
    client.setInsecure();
    HTTPClient http;
    http.setConnectTimeout(8000);
    http.setTimeout(10000);
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    http.setUserAgent("MissionControl-ESP32/1.0.2");
    if (!http.begin(client, url)) { error = "IMAGE REQUEST FAILED"; return false; }
    const int code = http.GET();
    if (code < 200 || code >= 300)
    {
        error = String("IMAGE HTTP ") + code;
        http.end();
        return false;
    }
    const int contentLength = http.getSize();
    if (contentLength > 0 && static_cast<size_t>(contentLength) > maximumBytes)
    {
        error = "IMAGE TOO LARGE";
        http.end();
        return false;
    }
    const size_t capacity = contentLength > 0
        ? static_cast<size_t>(contentLength) : maximumBytes;
    data = static_cast<uint8_t*>(heap_caps_malloc(
        capacity, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    if (data == nullptr) data = static_cast<uint8_t*>(malloc(capacity));
    if (data == nullptr)
    {
        error = "NOT ENOUGH MEMORY FOR IMAGE";
        http.end();
        return false;
    }
    BoundedImageStream stream(data, capacity);
    const int written = http.writeToStream(&stream);
    http.end();
    if (written <= 0 || stream.length() == 0 || stream.overflowed())
    {
        free(data);
        data = nullptr;
        error = "IMAGE DOWNLOAD FAILED";
        return false;
    }
    size = stream.length();
    return true;
}

bool WsjtxService::fetchPhoto(const String& targetCallsign)
{
    uint8_t* jpegData = nullptr;
    size_t jpegSize = 0;
    String error;
    Serial.println("[WSJT-X] Fetching callbook photo");
    if (!downloadImage(photoUrl, MAX_PHOTO_BYTES, jpegData, jpegSize, error))
    {
        Serial.println("[WSJT-X] " + error);
        return false;
    }
    if (jpegSize < 3 || jpegData[0] != 0xFF || jpegData[1] != 0xD8)
    {
        free(jpegData);
        Serial.println("[WSJT-X] Callbook photo is not JPEG");
        return false;
    }

    void* decoderStorage = heap_caps_malloc(
        sizeof(JPEGDEC), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (decoderStorage == nullptr)
        decoderStorage = heap_caps_malloc(
            sizeof(JPEGDEC), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    if (decoderStorage == nullptr) { free(jpegData); return false; }
    JPEGDEC* decoder = new (decoderStorage) JPEGDEC();
    if (!decoder->openRAM(jpegData, static_cast<int>(jpegSize), drawPhotoBlock))
    {
        decoder->~JPEGDEC();
        free(decoderStorage);
        free(jpegData);
        return false;
    }

    const uint16_t sourceWidth = decoder->getWidth();
    const uint16_t sourceHeight = decoder->getHeight();
    int scale = 0;
    uint8_t divisor = 1;
    // Decode enough detail for the expanded callbook-photo area. JPEGDEC
    // scales in powers of two, so retaining up to 400x300 avoids needlessly
    // reducing common 800x600 images to 200x150 before LVGL fits them.
    while ((sourceWidth / divisor > 400 || sourceHeight / divisor > 300) &&
           divisor < 8)
        divisor *= 2;
    if (divisor == 2) scale = JPEG_SCALE_HALF;
    else if (divisor == 4) scale = JPEG_SCALE_QUARTER;
    else if (divisor == 8) scale = JPEG_SCALE_EIGHTH;
    const uint16_t width = (sourceWidth + divisor - 1) / divisor;
    const uint16_t height = (sourceHeight + divisor - 1) / divisor;
    if (width > 420 || height > 320)
    {
        decoder->close();
        decoder->~JPEGDEC();
        free(decoderStorage);
        free(jpegData);
        return false;
    }
    uint16_t* pixels = static_cast<uint16_t*>(heap_caps_malloc(
        static_cast<size_t>(width) * height * sizeof(uint16_t),
        MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    if (pixels == nullptr)
        pixels = static_cast<uint16_t*>(malloc(
            static_cast<size_t>(width) * height * sizeof(uint16_t)));
    if (pixels == nullptr)
    {
        decoder->close();
        decoder->~JPEGDEC();
        free(decoderStorage);
        free(jpegData);
        return false;
    }
    PhotoDecodeContext context;
    context.pixels = pixels;
    context.width = width;
    context.height = height;
    decoder->setUserPointer(&context);
    const int result = decoder->decode(0, 0, scale);
    decoder->close();
    decoder->~JPEGDEC();
    free(decoderStorage);
    free(jpegData);
    if (!result) { free(pixels); return false; }
    if (photoPixels != nullptr) free(photoPixels);
    photoPixels = pixels;
    photoWidth = width;
    photoHeight = height;
    photoCallsign = targetCallsign;
    ++photoGeneration;
    Serial.print("[WSJT-X] Callbook photo ready: ");
    Serial.print(width);
    Serial.print("x");
    Serial.println(height);
    return true;
}

bool WsjtxService::fetchFlag(const String& targetCallsign)
{
    uint8_t* downloaded = nullptr;
    size_t downloadedSize = 0;
    String error;
    const String url = "https://flagcdn.com/w80/" + countryCode + ".png";
    if (!downloadImage(url, MAX_FLAG_BYTES, downloaded, downloadedSize, error))
    {
        Serial.println("[WSJT-X] Flag unavailable: " + error);
        return false;
    }
    if (downloadedSize < 24 || memcmp(downloaded, "\x89PNG\r\n\x1a\n", 8) != 0)
    {
        free(downloaded);
        return false;
    }
    const uint32_t width =
        (static_cast<uint32_t>(downloaded[16]) << 24) |
        (static_cast<uint32_t>(downloaded[17]) << 16) |
        (static_cast<uint32_t>(downloaded[18]) << 8) | downloaded[19];
    const uint32_t height =
        (static_cast<uint32_t>(downloaded[20]) << 24) |
        (static_cast<uint32_t>(downloaded[21]) << 16) |
        (static_cast<uint32_t>(downloaded[22]) << 8) | downloaded[23];
    if (width == 0 || height == 0 || width > 160 || height > 120)
    {
        free(downloaded);
        return false;
    }
    if (flagData != nullptr) free(flagData);
    flagData = downloaded;
    flagSize = downloadedSize;
    flagWidth = static_cast<uint16_t>(width);
    flagHeight = static_cast<uint16_t>(height);
    flagCallsign = targetCallsign;
    ++flagGeneration;
    return true;
}

void WsjtxService::releaseImages()
{
    if (photoPixels != nullptr) free(photoPixels);
    if (flagData != nullptr) free(flagData);
    photoPixels = nullptr;
    photoWidth = photoHeight = 0;
    flagData = nullptr;
    flagSize = 0;
    flagWidth = flagHeight = 0;
    photoCallsign = flagCallsign = "";
    ++photoGeneration;
    ++flagGeneration;
}

String WsjtxService::isoCountryCode(const String& country)
{
    String value = country;
    value.trim();
    value.toLowerCase();
    struct Mapping { const char* name; const char* code; };
    static const Mapping mappings[] = {
        {"united states", "us"}, {"usa", "us"}, {"canada", "ca"},
        {"mexico", "mx"}, {"england", "gb"}, {"united kingdom", "gb"},
        {"scotland", "gb"}, {"wales", "gb"}, {"northern ireland", "gb"},
        {"germany", "de"}, {"france", "fr"}, {"italy", "it"},
        {"spain", "es"}, {"portugal", "pt"}, {"netherlands", "nl"},
        {"belgium", "be"}, {"switzerland", "ch"}, {"austria", "at"},
        {"denmark", "dk"}, {"norway", "no"}, {"sweden", "se"},
        {"finland", "fi"}, {"iceland", "is"}, {"ireland", "ie"},
        {"poland", "pl"}, {"czech republic", "cz"}, {"czechia", "cz"},
        {"slovakia", "sk"}, {"hungary", "hu"}, {"romania", "ro"},
        {"bulgaria", "bg"}, {"croatia", "hr"}, {"slovenia", "si"},
        {"serbia", "rs"}, {"greece", "gr"}, {"turkey", "tr"},
        {"ukraine", "ua"}, {"russia", "ru"}, {"european russia", "ru"},
        {"japan", "jp"}, {"china", "cn"}, {"south korea", "kr"},
        {"republic of korea", "kr"}, {"india", "in"}, {"indonesia", "id"},
        {"philippines", "ph"}, {"thailand", "th"}, {"malaysia", "my"},
        {"australia", "au"}, {"new zealand", "nz"}, {"brazil", "br"},
        {"argentina", "ar"}, {"chile", "cl"}, {"colombia", "co"},
        {"peru", "pe"}, {"south africa", "za"}, {"israel", "il"}
    };
    for (const Mapping& mapping : mappings)
        if (value == mapping.name) return mapping.code;
    return "";
}

String WsjtxService::xmlValue(const String& xml, const char* tag)
{
    const String opening = String("<") + tag + ">";
    const String closing = String("</") + tag + ">";
    const int start = xml.indexOf(opening);
    if (start < 0) return "";
    const int contentStart = start + opening.length();
    const int end = xml.indexOf(closing, contentStart);
    if (end < 0) return "";
    String value = xml.substring(contentStart, end);
    value.replace("&amp;", "&");
    value.replace("&lt;", "<");
    value.replace("&gt;", ">");
    value.replace("&#39;", "'");
    value.replace("&quot;", "\"");
    return value;
}

String WsjtxService::urlEncode(const String& value)
{
    const char* hex = "0123456789ABCDEF";
    String encoded;
    encoded.reserve(value.length() * 3);
    for (size_t index = 0; index < value.length(); ++index)
    {
        const uint8_t character = static_cast<uint8_t>(value[index]);
        if ((character >= 'a' && character <= 'z') ||
            (character >= 'A' && character <= 'Z') ||
            (character >= '0' && character <= '9') ||
            character == '-' || character == '_' || character == '.')
            encoded += static_cast<char>(character);
        else
        {
            encoded += '%';
            encoded += hex[character >> 4];
            encoded += hex[character & 0x0F];
        }
    }
    return encoded;
}

bool WsjtxService::gridToCoordinates(
    const String& locator, double& latitude, double& longitude)
{
    if (locator.length() < 4) return false;
    String gridValue = locator;
    gridValue.toUpperCase();
    if (gridValue[0] < 'A' || gridValue[0] > 'R' ||
        gridValue[1] < 'A' || gridValue[1] > 'R' ||
        gridValue[2] < '0' || gridValue[2] > '9' ||
        gridValue[3] < '0' || gridValue[3] > '9') return false;
    longitude = -180.0 + (gridValue[0] - 'A') * 20.0 +
        (gridValue[2] - '0') * 2.0 + 1.0;
    latitude = -90.0 + (gridValue[1] - 'A') * 10.0 +
        (gridValue[3] - '0') + 0.5;
    if (gridValue.length() >= 6 && gridValue[4] >= 'A' && gridValue[4] <= 'X' &&
        gridValue[5] >= 'A' && gridValue[5] <= 'X')
    {
        longitude += (gridValue[4] - 'A') * (2.0 / 24.0) - 1.0 + (1.0 / 24.0);
        latitude += (gridValue[5] - 'A') * (1.0 / 24.0) - 0.5 + (0.5 / 24.0);
    }
    return true;
}

double WsjtxService::getDistanceKm() const
{
    double targetLat = 0.0;
    double targetLon = 0.0;
    const String locator = !grid.isEmpty() ? grid : callbookGrid;
    if (!gridToCoordinates(locator, targetLat, targetLon)) return -1.0;
    const double lat1 = configuration.latitude * DEG_TO_RAD;
    const double lat2 = targetLat * DEG_TO_RAD;
    const double deltaLat = (targetLat - configuration.latitude) * DEG_TO_RAD;
    const double deltaLon = (targetLon - configuration.longitude) * DEG_TO_RAD;
    const double a = sin(deltaLat / 2.0) * sin(deltaLat / 2.0) +
        cos(lat1) * cos(lat2) * sin(deltaLon / 2.0) * sin(deltaLon / 2.0);
    return 6371.0 * 2.0 * atan2(sqrt(a), sqrt(1.0 - a));
}

double WsjtxService::getBearingDegrees() const
{
    double targetLat = 0.0;
    double targetLon = 0.0;
    const String locator = !grid.isEmpty() ? grid : callbookGrid;
    if (!gridToCoordinates(locator, targetLat, targetLon)) return -1.0;
    const double lat1 = configuration.latitude * DEG_TO_RAD;
    const double lat2 = targetLat * DEG_TO_RAD;
    const double deltaLon = (targetLon - configuration.longitude) * DEG_TO_RAD;
    const double y = sin(deltaLon) * cos(lat2);
    const double x = cos(lat1) * sin(lat2) -
        sin(lat1) * cos(lat2) * cos(deltaLon);
    double bearing = atan2(y, x) * RAD_TO_DEG;
    if (bearing < 0.0) bearing += 360.0;
    return bearing;
}
