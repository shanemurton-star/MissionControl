#pragma once

#include <Arduino.h>
#include <WiFiUdp.h>

#include "../models/AppSettings.h"

class WsjtxService
{
public:
    ~WsjtxService();
    void begin(const AppSettings& settings);
    void configure(const AppSettings& settings);
    void poll();
    void updateLookup();

    const String& getCallsign() const { return callsign; }
    const String& getGrid() const { return grid; }
    const String& getMode() const { return mode; }
    const String& getState() const { return state; }
    const String& getCallbookName() const { return callbookName; }
    const String& getCallbookLocation() const { return callbookLocation; }
    const String& getCallbookCountry() const { return callbookCountry; }
    const String& getCallbookGrid() const { return callbookGrid; }
    const String& getLookupStatus() const { return lookupStatus; }
    const String& getProviderName() const { return providerName; }
    uint64_t getDialFrequency() const { return dialFrequency; }
    uint32_t getLastPacketMs() const { return lastPacketMs; }
    uint32_t getDataRevision() const { return dataRevision; }
    bool isListening() const { return listening; }
    bool hasCallbookData() const { return callbookValid; }
    bool hasPhoto() const { return photoPixels != nullptr && photoCallsign == callsign; }
    const uint16_t* getPhotoPixels() const { return photoPixels; }
    uint16_t getPhotoWidth() const { return photoWidth; }
    uint16_t getPhotoHeight() const { return photoHeight; }
    uint32_t getPhotoGeneration() const { return photoGeneration; }
    bool hasFlag() const { return flagData != nullptr && flagCallsign == callsign; }
    const uint8_t* getFlagData() const { return flagData; }
    size_t getFlagSize() const { return flagSize; }
    uint16_t getFlagWidth() const { return flagWidth; }
    uint16_t getFlagHeight() const { return flagHeight; }
    const String& getCountryCode() const { return countryCode; }
    uint32_t getFlagGeneration() const { return flagGeneration; }
    double getDistanceKm() const;
    double getBearingDegrees() const;

private:
    static constexpr uint32_t WSJTX_MAGIC = 0xADBCCBDA;
    static constexpr uint16_t MAX_PACKET_SIZE = 1024;
    static constexpr uint32_t LOOKUP_RETRY_MS = 60000UL;
    static constexpr size_t MAX_PHOTO_BYTES = 512UL * 1024UL;
    static constexpr size_t MAX_FLAG_BYTES = 32UL * 1024UL;

    bool startListener();
    void parsePacket(const uint8_t* data, size_t length);
    bool lookupHamQth(const String& targetCallsign);
    bool lookupQrz(const String& targetCallsign);
    bool fetchPhoto(const String& targetCallsign);
    bool fetchFlag(const String& targetCallsign);
    bool downloadImage(
        const String& url, size_t maximumBytes,
        uint8_t*& data, size_t& size, String& error);
    void releaseImages();
    bool httpGet(const String& url, String& response, int& responseCode);
    static String xmlValue(const String& xml, const char* tag);
    static String urlEncode(const String& value);
    static bool gridToCoordinates(const String& locator, double& latitude, double& longitude);
    static String isoCountryCode(const String& country);

    WiFiUDP udp;
    AppSettings configuration;
    uint8_t packetBuffer[MAX_PACKET_SIZE] = {};
    bool listening = false;
    bool lookupPending = false;
    bool lookupRunning = false;
    bool callbookValid = false;
    uint32_t listenerRetryMs = 0;
    uint32_t lookupRetryMs = 0;
    uint32_t lastPacketMs = 0;
    uint32_t dataRevision = 0;
    uint64_t dialFrequency = 0;
    String callsign;
    String grid;
    String mode;
    String state = "WAITING FOR WSJT-X";
    String providerName = "HAMQTH";
    String callbookName;
    String callbookLocation;
    String callbookCountry;
    String callbookGrid;
    String photoUrl;
    String countryCode;
    String lookupStatus = "WAITING FOR A SELECTED CONTACT";
    String sessionKey;
    uint16_t* photoPixels = nullptr;
    uint16_t photoWidth = 0;
    uint16_t photoHeight = 0;
    uint32_t photoGeneration = 0;
    String photoCallsign;
    uint8_t* flagData = nullptr;
    size_t flagSize = 0;
    uint16_t flagWidth = 0;
    uint16_t flagHeight = 0;
    uint32_t flagGeneration = 0;
    String flagCallsign;
};
