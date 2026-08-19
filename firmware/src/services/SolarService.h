#pragma once

#include <Arduino.h>
#include "../models/SolarData.h"

class SolarService
{
public:
    void begin();
    void update();
    bool isValid() const;
    bool isUpdating() const;
    const SolarData& getData() const;
    const String& getLastError() const;
    const char* getPropagationLabel() const;

private:
    static constexpr unsigned long REFRESH_MS = 5UL * 60UL * 1000UL;
    static constexpr unsigned long RETRY_MS = 60UL * 1000UL;
    static constexpr unsigned long REQUEST_TIMEOUT_MS = 15000UL;
    bool fetchJson(const char* url, String& payload);
    bool fetchAll();
    static bool timeReached(unsigned long target);

    SolarData data;
    String lastError;
    unsigned long nextUpdateMs = 0;
    bool valid = false;
    bool updating = false;
};
