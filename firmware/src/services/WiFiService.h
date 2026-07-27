#pragma once

#include <Arduino.h>
#include <WiFi.h>

#include "../models/AppSettings.h"

class WiFiService
{
public:
    void begin(const AppSettings& settings);
    void update();

    bool isConnected() const;

private:
    String ssid;
    String password;
    String hostname;

    uint32_t connectionStartMillis = 0;
    uint32_t lastRetryMillis = 0;

    bool connectionReported = false;

    static constexpr uint32_t CONNECTION_TIMEOUT_MS = 15000;
    static constexpr uint32_t RETRY_INTERVAL_MS = 30000;
};