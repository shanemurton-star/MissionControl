#include "WiFiService.h"

void WiFiService::begin(const AppSettings& settings)
{
    Serial.println();
    Serial.println("Starting WiFi service...");
    ssid = settings.wifiSsid;
    password = settings.wifiPassword;
    hostname = settings.hostname;
    if (ssid.isEmpty() || password.isEmpty())
    {
        Serial.println("WiFi credentials not configured.");
        return;
    }
    // Clear only the ESP32 WiFi driver's cached AP state. MissionControl keeps
    // its credentials in a separate Preferences namespace and immediately
    // reconnects from those saved application settings below.
    WiFi.disconnect(true, true);
    delay(200);
    WiFi.mode(WIFI_STA);
    if (!hostname.isEmpty()) WiFi.setHostname(hostname.c_str());
    WiFi.begin(ssid.c_str(), password.c_str());
    connectionStartMillis = millis();
    lastRetryMillis = connectionStartMillis;
    connectionReported = false;
    Serial.println("Connecting to WiFi");
}

void WiFiService::update()
{
    if (ssid.isEmpty() || password.isEmpty()) return;
    if (WiFi.status() == WL_CONNECTED)
    {
        if (!connectionReported)
        {
            connectionReported = true;
            Serial.println();
            Serial.println("WiFi connected!");
            Serial.print("Network: "); Serial.println(WiFi.SSID());
            Serial.print("Hostname: "); Serial.println(hostname.isEmpty() ? "(not set)" : hostname);
            Serial.print("IP address: "); Serial.println(WiFi.localIP());
            Serial.print("Gateway: "); Serial.println(WiFi.gatewayIP());
            Serial.print("DNS primary: "); Serial.println(WiFi.dnsIP(0));
            Serial.print("Signal strength: "); Serial.print(WiFi.RSSI()); Serial.println(" dBm");
        }
        return;
    }
    connectionReported = false;
    const uint32_t now = millis();
    if (now - connectionStartMillis < CONNECTION_TIMEOUT_MS ||
        now - lastRetryMillis < RETRY_INTERVAL_MS) return;
    Serial.println();
    Serial.println("WiFi not connected. Retrying...");
    WiFi.disconnect();
    WiFi.begin(ssid.c_str(), password.c_str());
    connectionStartMillis = now;
    lastRetryMillis = now;
}

bool WiFiService::isConnected() const { return WiFi.status() == WL_CONNECTED; }

bool WiFiService::isNetworkReady() const
{
    return isConnected() && static_cast<uint32_t>(WiFi.localIP()) != 0;
}

int WiFiService::scanNetworks()
{
    Serial.println("Pausing WiFi connection for network scan...");
    WiFi.disconnect();
    delay(150);
    WiFi.mode(WIFI_STA);
    const int found = WiFi.scanNetworks(false, true);
    Serial.print("WiFi scan result: "); Serial.println(found);
    return found;
}

void WiFiService::finishNetworkScan()
{
    WiFi.scanDelete();
    connectionReported = false;
    connectionStartMillis = millis();
    lastRetryMillis = connectionStartMillis;
    Serial.println("Resuming WiFi connection after scan...");
    WiFi.begin(ssid.c_str(), password.c_str());
}
