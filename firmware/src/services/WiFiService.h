#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include "../config/Settings.h"
#include "../config/secrets.h"

class WiFiService
{
public:

    void begin();

    bool isConnected();

private:

const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWORD;
};