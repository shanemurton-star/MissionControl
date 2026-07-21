#pragma once

#include <Arduino.h>
#include <WiFi.h>

class WiFiService
{
public:

    void begin();

    bool isConnected();

private:

    const char* ssid = "";
    const char* password = "";
};