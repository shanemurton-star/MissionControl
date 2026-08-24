#pragma once

#include <Arduino.h>
#include "../models/SolarData.h"

class SolarService
{
public:
    ~SolarService();
    void begin();
    void update(bool allowCoronaImage = true);
    bool isValid() const;
    bool isUpdating() const;
    const SolarData& getData() const;
    const String& getLastError() const;
    const char* getPropagationLabel() const;
    bool hasCoronaImage() const;
    const uint16_t* getCoronaPixels() const;
    uint16_t getCoronaWidth() const;
    uint16_t getCoronaHeight() const;
    uint32_t getCoronaGeneration() const;
    const String& getCoronaError() const;

private:
    static constexpr unsigned long REFRESH_MS = 15UL * 60UL * 1000UL;
    static constexpr unsigned long RETRY_MS = 60UL * 1000UL;
    static constexpr unsigned long REQUEST_TIMEOUT_MS = 8000UL;
    static constexpr size_t MAX_CORONA_JPEG_SIZE = 192UL * 1024UL;
    bool fetchJson(const char* url, String& payload);
    bool fetchAll();
    bool fetchCoronaImage();
    void releaseCoronaImage();
    static bool timeReached(unsigned long target);

    SolarData data;
    String lastError;
    unsigned long nextUpdateMs = 0;
    unsigned long nextCoronaUpdateMs = 0;
    bool valid = false;
    bool updating = false;

    uint16_t* coronaPixels = nullptr;
    uint16_t coronaWidth = 0;
    uint16_t coronaHeight = 0;
    uint32_t coronaGeneration = 0;
    String coronaError;
};
