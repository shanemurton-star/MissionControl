#include "WiFiService.h"

void WiFiService::begin(
    const AppSettings& settings)
{
    Serial.println();
    Serial.println("Starting WiFi service...");

    ssid = settings.wifiSsid;
    password = settings.wifiPassword;
    hostname = settings.hostname;

    if (ssid.isEmpty() ||
        password.isEmpty())
    {
        Serial.println(
            "WiFi credentials not configured.");

        return;
    }

    WiFi.disconnect();
    WiFi.mode(WIFI_STA);

    if (!hostname.isEmpty())
    {
        WiFi.setHostname(
            hostname.c_str());
    }

    WiFi.begin(
        ssid.c_str(),
        password.c_str());

    connectionStartMillis = millis();
    lastRetryMillis = connectionStartMillis;
    connectionReported = false;

    Serial.println("Connecting to WiFi");
}

void WiFiService::update()
{
    if (ssid.isEmpty() ||
        password.isEmpty())
    {
        return;
    }

    if (WiFi.status() == WL_CONNECTED)
    {
        if (!connectionReported)
        {
            connectionReported = true;

            Serial.println();
            Serial.println("WiFi connected!");

            Serial.print("Network: ");
            Serial.println(WiFi.SSID());

            Serial.print("Hostname: ");
            Serial.println(
                hostname.isEmpty()
                    ? "(not set)"
                    : hostname);

            Serial.print("IP address: ");
            Serial.println(WiFi.localIP());

            Serial.print("Signal strength: ");
            Serial.print(WiFi.RSSI());
            Serial.println(" dBm");
        }

        return;
    }

    connectionReported = false;

    const uint32_t currentMillis =
        millis();

    if (currentMillis -
            connectionStartMillis <
        CONNECTION_TIMEOUT_MS)
    {
        return;
    }

    if (currentMillis -
            lastRetryMillis <
        RETRY_INTERVAL_MS)
    {
        return;
    }

    Serial.println();
    Serial.println(
        "WiFi not connected. Retrying...");

    WiFi.disconnect();

    WiFi.begin(
        ssid.c_str(),
        password.c_str());

    connectionStartMillis =
        currentMillis;

    lastRetryMillis =
        currentMillis;
}

bool WiFiService::isConnected() const
{
    return WiFi.status() ==
           WL_CONNECTED;
}
